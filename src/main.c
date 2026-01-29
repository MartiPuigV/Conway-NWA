#include "storage.h"
#include <eadk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define F 5             // Scaling factor
#define H (240 / F)
#define W (320 / F)

#define IDX(x, y) ((y) * W + (x))
#define MIN(a, b) (((a) < (b)) ? (a)  : (b))
#define MAX(a, b) (((a) > (b)) ? (a)  : (b))

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Conway";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* Constants and types */
static const int menu_ms_delay = 250;
static const char* SAVE_FILE = "pattern.cwp";
static const eadk_color_t CURSOR_COLOR = 0xFCD5; // Pink, all channels
// Choose a color that uses all channels, or low F values will
// make it hard to see where the cursor is aligned. Full red
// cursor appears to end higher than it should since red is
// the first channel
static int COLOR_IDX = 0;
static eadk_color_t CELL_COLOR = 0xFFFF;
static eadk_color_t DEAD_COLOR = 0x0000;
static eadk_color_t DIFF_COLOR = 0xFFFF;

typedef uint8_t cell_t;

typedef struct {
    cell_t cells[H * W];
} Grid;

static const uint8_t rules[2][10] = {
    // Precompute the game rules for next generation
    [0] = { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    [1] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 }
};

static const eadk_color_t cell_colors[3] = {
    0xFFFF, // White
    0xBECA, // Green
    0xFDCF, // Peach
};

static const eadk_color_t dead_colors[3] = {
    0x0000, // Black
    0x4AA4, // Dark green
    0x6246, // Dark brown
};

/* Functions */
static inline uint8_t get_neighbors(const cell_t* cells, size_t x, size_t y) {
    // Get neighbor count of cell at (x; y)
    size_t n = 0;
    const size_t row = y * W;
    const size_t row_up = (y > 0) ? row - W : 0;        // Maybe not accurate at borders idk
    const size_t row_dn = (y + 1 < H) ? row + W : row;  // Same issue

    if (y > 0) {
        if (x > 0)          n += cells[row_up + x - 1];
        n += cells[row_up + x];
        if (x+1 < W)        n += cells[row_up + x + 1];
    }

    if (x > 0)              n += cells[row + x - 1];
    if (x+1 < W)            n += cells[row + x + 1];

    if (y+1 < H) {
        if (x > 0)          n += cells[row_dn + x - 1];
        n += cells[row_dn + x];
        if (x+1 < W)        n += cells[row_dn + x + 1];
    }

    return n;
}

static inline void step(cell_t* restrict p_buffer_main, cell_t* restrict p_buffer_alt) {
    size_t i = 0;
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++, i++) {
            uint8_t n = get_neighbors(p_buffer_main, x, y);
            p_buffer_alt[i] = rules[p_buffer_main[i]][n];

            if (p_buffer_alt[i] != p_buffer_main[i]) {
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ x*F, y*F, F, F },
                    (DIFF_COLOR) * p_buffer_alt[i] + DEAD_COLOR
                );
            }
        }
    }
}

static inline void display_init_cells(const cell_t* cells) {
    size_t i = 0;
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++, i++) {
            eadk_display_push_rect_uniform(
                (eadk_rect_t){ x*F, y*F, F, F },
                (DIFF_COLOR) * cells[i] + DEAD_COLOR
            );
        }
    }
}

static inline void cells_insert_pattern(cell_t* p_buffer_main, const char* pattern, size_t size, eadk_point_t pos) {
    if (size < 3) {
        // File should be at least 3 bytes
        return;
    }

    cell_t color = (pattern[0] & (1 << 7)) != 0;
    size_t w = ((pattern[0] & 1) << 8)+ pattern[1];
    size_t px = 0;

    for (size_t i = 2; i < size; i++) {
        char rle = pattern[i];
        size_t x = px % w;
        size_t y = px / w;

        if (y + 1 == H) {
            return;
        }

        while (rle > 0) {
            size_t segment = MIN(w - x, rle);
            memset(p_buffer_main + IDX(pos.x + x, pos.y + y), color, MIN(segment, W - x));
            rle -= segment;
            px += segment;
        }

        color = !color;
    }
}

static char* rle_encode(const cell_t* cells, size_t size, size_t* out_size) {
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

static char* yank_pattern(const cell_t* p_buffer_main, size_t* out_size, eadk_rect_t area) {
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
        memcpy(cells + (row * w), p_buffer_main + IDX(x, y + row), w);
    }

    size_t rle_size = 0;
    char* rle = rle_encode(cells, w * h, &rle_size);

    char* res = malloc(sizeof(char) * (rle_size + 2));
    *out_size = rle_size + 2; // Let caller know result size;
    // Copy at byte [3 ->
    memcpy(res+2, rle, rle_size);
    // Set header values
    res[0] = (p_buffer_main[IDX(x, y)] << 7) | (area.width > 0xFF);
    res[1] = (area.width & 0xFF);

    free(rle);

    return res;
}

static inline void _menu_color(cell_t* p_buffer_main, int col) {
    COLOR_IDX = (COLOR_IDX + 3 + col) % 3;
    CELL_COLOR = cell_colors[COLOR_IDX];
    DEAD_COLOR = dead_colors[COLOR_IDX];
    DIFF_COLOR = CELL_COLOR - DEAD_COLOR;
    display_init_cells(p_buffer_main);
    eadk_timing_msleep(menu_ms_delay);
}

static inline void _menu_mod(cell_t* buffer, eadk_point_t cursor, int mod) {
    eadk_color_t c = (mod == 1) ? 1 : 0; // Turns (-1, 1) into (0, 1)
    buffer[IDX(cursor.x, cursor.y)] = c;
    eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*F, cursor.y*F, F, F }, CELL_COLOR * c);
}

static inline void _menu_paste_pattern(cell_t* buffer, eadk_point_t cursor) {
    size_t pattern_size = 0;
    const char* pattern = extapp_fileRead(SAVE_FILE, &pattern_size);

    cells_insert_pattern(buffer, pattern, pattern_size, cursor); // Insert pattern into array
    display_init_cells(buffer); // Update display with newly pasted cells
    eadk_timing_msleep(menu_ms_delay);
}

int main(int argc, char * argv[]) {
    // Main "global" variables
    Grid buffer_main = {0};
    Grid buffer_alt = {0};
    eadk_point_t cursor = { W/2, H/2 };

    // Selection tool for copy/paste
    eadk_rect_t selection = {0};
    uint8_t cursor_speed = 4;

    // First screen reset (black)
    eadk_display_push_rect_uniform(eadk_screen_rect, 0x0);

    // Define pointers to both buffers for functions (pass pointer not array)
    cell_t* p_buffer_main = buffer_main.cells;
    cell_t* p_buffer_alt = buffer_alt.cells;

    // Optional: load pattern from external_data
    // cells_insert_pattern(p_buffer_main, eadk_external_data, eadk_external_data_size, 0, 0);
    // display_init_cells(p_buffer_main);

    // Menu flags
    bool pause = true;
    bool select = false;
    int frame_ms = 0;

    // Avoid app opening to cause keydown(OK)
    eadk_timing_msleep(menu_ms_delay);

    while (true) {
        // Add option to toggle vblank ?
        // eadk_display_wait_for_vblank();
        eadk_keyboard_state_t kb = eadk_keyboard_scan();

        if (eadk_keyboard_key_down(kb, eadk_event_ok) && !select) {
            pause = !pause;
            eadk_timing_msleep(menu_ms_delay); // Add delay to be usable
        }

        // Handle pause / menu
        if (pause) {
            // Handle movement and cell modifications
            int x   = (eadk_keyboard_key_down(kb, eadk_event_right)     - eadk_keyboard_key_down(kb, eadk_event_left));
            int y   = (eadk_keyboard_key_down(kb, eadk_event_down)      - eadk_keyboard_key_down(kb, eadk_event_up));
            int mod = (eadk_keyboard_key_down(kb, eadk_event_toolbox)   - eadk_keyboard_key_down(kb, eadk_event_backspace));
            int ms  = (eadk_keyboard_key_down(kb, eadk_event_plus)      - eadk_keyboard_key_down(kb, eadk_event_minus));
            int col = eadk_keyboard_key_down(kb, eadk_event_division);

            if (col) { // Color palette change
                _menu_color(p_buffer_main, col);
            }

            // Modification flag
            if (mod != 0 && !select) { // Don't modify when selecting cells
                _menu_mod(p_buffer_main, cursor, mod);
            }

            if (ms != 0) { // Change time interval between frames
                frame_ms = MAX(0, frame_ms + 5*ms);
                eadk_timing_msleep(menu_ms_delay);
            }

            // Update cursor position
            cursor.x = (cursor.x + x + W) % W;
            cursor.y = (cursor.y + y + H) % H;

            // Handle selections
            if (eadk_keyboard_key_down(kb, eadk_event_shift)) {
                if (select) { // Selecting second point
                    uint16_t min_x = MIN(cursor.x, selection.x);
                    uint16_t min_y = MIN(cursor.y, selection.y);
                    selection.width = (cursor.x > selection.x) ? (cursor.x - selection.x + 1) : (selection.x - cursor.x + 1);
                    selection.height = (cursor.y > selection.y) ? (cursor.y - selection.y + 1) : (selection.y - cursor.y + 1);
                    selection.x = min_x;
                    selection.y = min_y;

                    size_t yank_size = 0;
                    char* selected_cells = yank_pattern(p_buffer_main, &yank_size, selection);

                    // Avoid overwriting savefile if no pattern was yanked
                    if (selected_cells && yank_size > 0) {
                        extapp_fileErase(SAVE_FILE);
                        // Erase previous file to avoid getting multiple files with same name
                        extapp_fileWrite(SAVE_FILE, selected_cells, yank_size);

                        free(selected_cells);
                    }
                } else { // Selecting first point
                    selection.x = cursor.x;
                    selection.y = cursor.y;
                }

                select = !select;
                eadk_timing_msleep(menu_ms_delay); // Pause to avoid instant toggling
            } // end if (shift)

            // Pasting pattern from memory
            if (eadk_keyboard_key_down(kb, eadk_event_ans) && extapp_fileExists(SAVE_FILE)) {
                _menu_paste_pattern(p_buffer_main, cursor);
            }

            // Display cursor and selection corners
            eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*F, cursor.y*F, F, F }, CURSOR_COLOR);
            if (select) {
                eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*F, selection.y*F, F, F }, CURSOR_COLOR);
                eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*F, selection.y*F, F, F }, CURSOR_COLOR);
                eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*F, cursor.y*F, F, F }, CURSOR_COLOR);
            }

            // Framerate in menu, resulting in speed of cursor. Switch to delta time please
            eadk_timing_msleep(menu_ms_delay/cursor_speed);

            // Clear cursor and selection corners
            eadk_display_push_rect_uniform(
                (eadk_rect_t){ cursor.x*F, cursor.y*F, F, F },
                (DIFF_COLOR) * p_buffer_main[ IDX( cursor.x, cursor.y ) ] + DEAD_COLOR
            );

            if (select) {
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ selection.x*F, selection.y*F, F, F },
                    (DIFF_COLOR) * p_buffer_main[ IDX( selection.x, selection.y ) ] + DEAD_COLOR
                );
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ cursor.x*F, selection.y*F, F, F },
                    (DIFF_COLOR) * p_buffer_main[ IDX( cursor.x, selection.y ) ] + DEAD_COLOR
                );
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ selection.x*F, cursor.y*F, F, F },
                    (DIFF_COLOR) * p_buffer_main[ IDX( selection.x, cursor.y ) ] + DEAD_COLOR
                );
            }
            // end if (pause)
        } else { // !pause, AKA run simulation
            step(p_buffer_main, p_buffer_alt);
            cell_t* tmp = p_buffer_main;
            p_buffer_main = p_buffer_alt;
            p_buffer_alt = tmp;
            eadk_timing_msleep(frame_ms);
        }
    }

    return 0;
}
