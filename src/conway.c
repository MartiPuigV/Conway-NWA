#include "macros.h"

#include <eadk.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t cell_t;

extern int  SCALE, STRICT_PASTE;
extern eadk_color_t DIFF_COLOR, DEAD_COLOR;

const uint8_t rules[2][10] = {
    // Precompute the game rules for next generation
    [0] = { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    [1] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 }
};

uint8_t get_neighbors(const cell_t* cells, size_t x, size_t y) {
    // Get neighbor count of cell at (x; y)
    size_t n = 0;

    const size_t row = y * W;
    const size_t row_up = (y > 0) ? row - W : 0;        // Should accurate at borders idk
    const size_t row_dn = (y + 1 < H) ? row + W : row;  // Same as above

    const bool y_0   = y > 0;
    const bool x_0   = x > 0;
    const bool xp1_W = x + 1 < W;
    const bool yp1_H = y + 1 < H;

    n += cells[row_up + x - 1] * y_0 * x_0;
    n += cells[row_up + x    ] * y_0;
    n += cells[row_up + x + 1] * y_0 * xp1_W;

    n += cells[row + x - 1] * x_0;
    n += cells[row + x + 1] * xp1_W;

    n += cells[row_dn + x - 1] * yp1_H * x_0;
    n += cells[row_dn + x    ] * yp1_H;
    n += cells[row_dn + x + 1] * yp1_H * xp1_W;

    return n;
}

void step(cell_t* restrict buffer_main, cell_t* restrict buffer_alt) {
    /*
    * Alt buffer is two rows in size, and contains the updated grid
    * When more than two rows have been computed, overwrite the main
    * buffer safely, as it doesn't affect other cells, and overwrite
    * the alt buffer one cell at a time.
    */

    size_t i = 0;

    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++, i++) {
            uint8_t neighbors = get_neighbors(buffer_main, x, y);
            uint8_t new_cell = rules [ buffer_main[i] ] [ neighbors ];

            // Alt buffer magic
            size_t idx = (y%2) * W + x;
            if (y > 1) buffer_main[i - (2 * W)] = buffer_alt[idx];
            buffer_alt[idx] = new_cell;

            if (new_cell == buffer_main[i]) continue;

            // Only display stuff that actually changes
            eadk_display_push_rect_uniform(
                (eadk_rect_t){ x*SCALE, y*SCALE, SCALE, SCALE },
                (DIFF_COLOR) * new_cell + DEAD_COLOR
            );
        }
    }

    // Copy leftover of buffer_alt to buffer_main's last two rows
    memcpy(buffer_main + i - (2 * W), buffer_alt, 2 * W * sizeof(cell_t));
}

void display_draw_cells(const cell_t* cells) {
    size_t i = 0;

    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++, i++) {
            eadk_display_push_rect_uniform(
                (eadk_rect_t){ x*SCALE, y*SCALE, SCALE, SCALE },
                (DIFF_COLOR) * cells[i] + DEAD_COLOR
            );
        }
    }
}

void cells_insert_pattern(cell_t* buffer_main, const char* pattern, size_t size, eadk_point_t pos) {
    if (size < 3) {
        // File should be at least 3 bytes
        return;
    }

    cell_t color = (pattern[0] >> 7) & 1;
    size_t w = ((pattern[0] & 1) << 8) + pattern[1];
    size_t px_idx = 0;
    size_t x, y, segment;

    for (size_t i = 2; i < size; i++) {
        char rle = pattern[i];

        if (!STRICT_PASTE && color == 0) {
            px_idx += rle;
            color = 1;
            continue;
        }

        while (rle > 0) {
            x = px_idx % w;
            y = px_idx / w;
            segment = MIN(w - x, rle);
            memset(buffer_main + IDX(pos.x + x, pos.y + y), color, MIN(segment, W - x));
            rle -= segment;
            px_idx += segment;

            if (y + 1 == H) {
                return;
            }
        }

        color = !color;
    }
}

char* rle_encode(const cell_t* cells, size_t size, size_t* out_size) {
    // Returns RLE encoded cells. Bit is not specified, alternates based on first bit
    // Caller of function knows to implement starting bit in 2 byte header
    // Free the malloc'd char* it returns!

    size_t streak = 1;
    cell_t c = cells[0];
    char* rle = malloc(sizeof(char) * (2*size)); // Allocate enough for worst case scenario
    size_t idx = 0;

    for (size_t i = 1; i < size; i++) {
        if (cells[i] == c) {
            streak++;
            if (streak == 256) {
                rle[idx++] = 255;
                rle[idx++] = 0;
                // Add max value for current color and skip next with 0
                streak = 1;
            }
        } else {
            rle[idx++] = streak;
            streak = 1;
            c = cells[i];
        }
    }

    rle[idx++] = streak;

    rle = realloc(rle, idx); // Reallocate to actual size;
    *out_size = idx;

    return rle;
}

char* yank_pattern(const cell_t* buffer_main, size_t* out_size, eadk_rect_t area) {
    // Could just use a screen yank but it's safer to copy
    // from cells and not accidentaly yank some UI pixels
    // (and probably faster too!)
    // Returns a char* .cwp file contents
    // Free result!

    size_t x = area.x, y = area.y, w = area.width, h = area.height;

    if (x + w > W || y + h > H) {
        // Yanking is out of bounds
        *out_size = 0;
        return NULL;
    }

    cell_t cells[w * h];

    for (size_t row = 0; row < h; row++) {
        memcpy(cells + (row * w), buffer_main + IDX(x, y + row), w);
    }

    size_t rle_size = 0;
    char* rle = rle_encode(cells, w * h, &rle_size);

    char* res = malloc(sizeof(char) * (rle_size + 2));
    *out_size = rle_size + 2; // Let caller know result size;
    // Copy at byte [3 ->
    memcpy(res+2, rle, rle_size);
    // Set header values
    res[0] = (buffer_main[IDX(x, y)] << 7) | (area.width > 0xFF);
    res[1] = (area.width & 0xFF);

    free(rle);

    return res;
}

