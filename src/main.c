#include "storage.h"
#include <eadk.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Declare config before macros

typedef struct {
    int scale_idx;
    int color_idx;
    int frame_ms;
    int strict_paste;
} Config;

static const char* CONFIG_FILE = "conway.conf";

Config load_config(const char* config_file) {
    Config default_cfg = (Config){ 2, 0, 50, 1 };

    if (extapp_fileExists(config_file)) {
        size_t size = 0;
        const char* raw = extapp_fileRead(config_file, &size);

        if (size < 4) {
            // Should contain at least 4 bytes
            return default_cfg;
        }

        return (Config){ raw[0], raw[1], raw[2], raw[3] };
    }

    return default_cfg;
}

Config CONFIG;
int SCALE_IDX;
int SCALE;
int COLOR_IDX;
int FRAME_MS;
int STRICT_PASTE;

/* Macros */
#define H (240 / SCALE)
#define W (320 / SCALE)

#define IDX(x, y) ((y) * W + (x))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Conway";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

/* Constants and types */
static const int menu_ms_delay = 250;
static const int AREA_MAX = 160 * 120;
// The calculator's memory, with no other files, can support around 6 patterns of
// size AREA_MAX, with worst RLE configuration (alternating pixels)

static const eadk_color_t CURSOR_COLOR = 0xFCD5; // Pink, all channels
// Choose a color that uses all channels, or low SCALE values will
// make it hard to see where the cursor is aligned. Full red
// cursor appears to end higher than it should since red is
// the first channel
static eadk_color_t CELL_COLOR = 0xFFFF;
static eadk_color_t DEAD_COLOR = 0x0000;
static eadk_color_t DIFF_COLOR = 0xFFFF;
static char SAVE_FILE[] = "pattern0.cwp";

typedef uint8_t cell_t;

static const uint8_t rules[2][10] = {
    // Precompute the game rules for next generation
    [0] = { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    [1] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 }
};

static const int scales[5] = { 1, 2, 4, 5, 8 };

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

static const eadk_event_t numpad[10] = {
    48, 42, 43, 44, 36, 37, 38, 30, 31, 32 // Numpad events, 0-9
};

static inline void display_message(const char* message, size_t len) {
    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);
    eadk_display_draw_string(
        message,
        (eadk_point_t){ EADK_SCREEN_WIDTH/2 - 5*len, EADK_SCREEN_HEIGHT/2 - 10 },
        true, CELL_COLOR, DEAD_COLOR
    );
    eadk_timing_msleep(menu_ms_delay);
}

static const int eadk_event_to_numpad(eadk_event_t event) {
    // Translates the event ID to [0-9] if event is a numpad key else -1
    for (size_t i = 0; i < 10; i++) {
        if (numpad[i] == event) return i;
    }

    return -1;
}

static inline int _menu_await_numpad() {
    int32_t timeout;
    eadk_event_t key;
    int numpad;

    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);
    display_message("Select number [0-9]", 19);

    while (true) {
        key = eadk_event_get(&timeout);
        numpad = eadk_event_to_numpad(key);
        if (numpad != -1) return numpad;
    }
}

/* Functions */
static inline uint8_t get_neighbors(const cell_t* cells, size_t x, size_t y) {
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

static inline void step(cell_t* restrict buffer_main, cell_t* restrict buffer_alt) {
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

static inline void display_draw_cells(const cell_t* cells) {
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

static inline void cells_insert_pattern(cell_t* buffer_main, const char* pattern, size_t size, eadk_point_t pos) {
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

static char* yank_pattern(const cell_t* buffer_main, size_t* out_size, eadk_rect_t area) {
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

static inline void _menu_color(cell_t* buffer_main) {
    COLOR_IDX = (COLOR_IDX + 1) % 3;
    CELL_COLOR = cell_colors[COLOR_IDX];
    DEAD_COLOR = dead_colors[COLOR_IDX];
    DIFF_COLOR = CELL_COLOR - DEAD_COLOR;
    display_draw_cells(buffer_main);
    eadk_timing_msleep(menu_ms_delay);
}

static inline void _menu_mod(cell_t* buffer, eadk_point_t cursor, int mod) {
    eadk_color_t c = (mod == 1) ? 1 : 0; // Turns (-1, 1) into (0, 1)
    buffer[IDX(cursor.x, cursor.y)] = c;
    eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE }, (CELL_COLOR - DEAD_COLOR) * c + DEAD_COLOR);
}

static inline void _menu_paste_pattern(cell_t* buffer, eadk_point_t cursor, int savefile_idx) {
    size_t pattern_size = 0;
    SAVE_FILE[7] = '0'+savefile_idx;

    if (!extapp_fileExists(SAVE_FILE)) {
        display_message("That savefile doesn't exist!", 28);
        eadk_timing_msleep(menu_ms_delay);
        display_draw_cells(buffer);
        return;
    }

    const char* pattern = extapp_fileRead(SAVE_FILE, &pattern_size);

    cells_insert_pattern(buffer, pattern, pattern_size, cursor); // Insert pattern into array
    display_draw_cells(buffer); // Update display with newly pasted cells
    eadk_timing_msleep(menu_ms_delay);
}

static inline void _menu_save_config() {
    const char config[4] = { (char)SCALE_IDX, (char)COLOR_IDX, (char)FRAME_MS, (char)STRICT_PASTE };
    if (extapp_fileExists(CONFIG_FILE)) extapp_fileErase(CONFIG_FILE);
    extapp_fileWrite(CONFIG_FILE, config, 4);
    display_message("Saved config", 12);
    eadk_timing_msleep(menu_ms_delay);
}

static inline bool _menu_copy_pattern(cell_t* buffer, eadk_rect_t area, int savefile_idx) {
    if (area.width * area.height > AREA_MAX) {
        display_message("Area too large", 14);
        return false;
    }

    size_t yank_size = 0;
    char* selected_cells = yank_pattern(buffer, &yank_size, area);
    SAVE_FILE[7] = '0'+savefile_idx;
    // It's OK to overwrite because every

    // Avoid overwriting savefile if no pattern was yanked
    if (selected_cells && yank_size > 0) {
        if (extapp_fileExists(SAVE_FILE)) extapp_fileErase(SAVE_FILE);
        // Erase previous file to avoid getting multiple files with same name
        extapp_fileWrite(SAVE_FILE, selected_cells, yank_size);

        free(selected_cells);
    }

    // If not enough space, it won't create the file.
    // This only fails if saving to an already existing file and the memory is full.
    return extapp_fileExists(SAVE_FILE);
}

static inline void _menu_scale(int sc) {
    SCALE_IDX = (SCALE_IDX + 5 + sc) % 5;

    char msg[] = "[,] Scale:  ";
    msg[1] -= sc;
    // Some magic ASCII. Offset ',' by -sc.
    // + : 43
    // , : 44
    // - : 45
    msg[11] = '0' + scales[SCALE_IDX];
    // A classic

    display_message(msg, 12);
    eadk_timing_msleep(menu_ms_delay);
}

static inline void _menu_ms(int ms) {
    FRAME_MS = MIN(MAX(0, FRAME_MS + 10*ms), 250); // Clamp in [0, 250] range (min. of 4 FPS)

    char msg[] = "[,]     ms / frame";
    msg[1] -= ms; // Same magic as above
    msg[4] = (FRAME_MS / 100)      + '0';
    msg[5] = (FRAME_MS / 10 ) % 10 + '0';
    msg[6] = (FRAME_MS % 10 )      + '0';

    display_message(msg, 18);
    eadk_timing_msleep(menu_ms_delay / 2);
}

int main(int argc, char* argv[]) {
    // Load config
    CONFIG = load_config(CONFIG_FILE);
    SCALE_IDX = CONFIG.scale_idx;
    SCALE = scales[SCALE_IDX % 5];
    COLOR_IDX = CONFIG.color_idx;
    FRAME_MS = CONFIG.frame_ms;
    STRICT_PASTE = CONFIG.strict_paste;

    CELL_COLOR = cell_colors[COLOR_IDX];
    DEAD_COLOR = dead_colors[COLOR_IDX];
    DIFF_COLOR = CELL_COLOR - DEAD_COLOR;

    // Main "global" variables
    eadk_point_t cursor = { W/2, H/2 };

    // Selection tool for copy/paste
    eadk_rect_t selection = {0};
    uint8_t cursor_speed = 4;

    // First screen reset
    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);

    // Allocate buffers
    cell_t* buffer_main     = calloc(H * W, sizeof(cell_t));
    cell_t* buffer_alt      = calloc(2 * W, sizeof(cell_t));

    // Should not happen ... but if it does, starting simulation will reset!
    if (buffer_main == NULL || buffer_alt == NULL) {
        eadk_display_draw_string("alloc failed... quit app", (eadk_point_t){ 0, 0 }, true, eadk_color_red, eadk_color_black);
        eadk_timing_msleep(5000);
    }

    // Optional: load pattern from external_data
    // cells_insert_pattern(buffer_main, eadk_external_data, eadk_external_data_size, 0, 0);
    display_draw_cells(buffer_main);

    // Menu flags
    bool pause = true;
    bool select = false;
    bool step_lock = 0;

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
            int sc  = (eadk_keyboard_key_down(kb, eadk_event_left_parenthesis) - eadk_keyboard_key_down(kb, eadk_event_right_parenthesis));
            // Scale

            // Color palette change
            if (eadk_keyboard_key_down(kb, eadk_event_alpha)) {
                _menu_color(buffer_main);
            }

            // Modification flag
            if (mod != 0) { // Don't modify when selecting cells
                if (mod == -1 && select) { // Shift + backspace (clear)
                    memset(buffer_main, 0, W * H * sizeof(cell_t));
                    select = false;
                    // No need to call display_draw_cells
                    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);
                } else {
                    _menu_mod(buffer_main, cursor, mod);
                }
            }

            if (ms != 0) { // Change time interval between frames
                _menu_ms(ms);
                display_draw_cells(buffer_main);
            }

            if (sc != 0) {
                _menu_scale(sc);
                display_draw_cells(buffer_main);
            }

            // Copy entire screen
            if (eadk_keyboard_key_down(kb, eadk_event_multiplication)) {
                int numpad = _menu_await_numpad();
                bool status = _menu_copy_pattern(buffer_main, (eadk_rect_t){ 0, 0, W, H }, numpad);

                if (status) {
                    display_message("Copied entire screen", 20);
                }

                eadk_timing_msleep(menu_ms_delay);
                display_draw_cells(buffer_main);
            }

            // Change pasting mode Strict/Normal
            if (eadk_keyboard_key_down(kb, eadk_event_division)) {
                STRICT_PASTE = !STRICT_PASTE;
                display_message(STRICT_PASTE ? "Pasting: Strict mode" : "Pasting: Transparent", 20);
                eadk_timing_msleep(menu_ms_delay);
                display_draw_cells(buffer_main);
            }

            // Pasting pattern from memory
            if (eadk_keyboard_key_down(kb, eadk_event_ans)) {
                int numpad = _menu_await_numpad();
                _menu_paste_pattern(buffer_main, cursor, numpad);
            }

            // Save config
            if (eadk_keyboard_key_down(kb, eadk_event_exe)) {
                _menu_save_config();
                display_draw_cells(buffer_main);
            }

            // Handle stepping
            if (eadk_keyboard_key_down(kb, eadk_event_back) && !step_lock) {
                step(buffer_main, buffer_alt);
                // No wait since key has to be released
            }

            step_lock = eadk_keyboard_key_down(kb, eadk_event_back);

            // Update cursor position
            if (x) cursor.x = (cursor.x + x + W) % W;
            if (y) cursor.y = (cursor.y + y + H) % H;

            // Handle selections
            if (eadk_keyboard_key_down(kb, eadk_event_shift)) {
                if (select) { // Selecting second point
                    uint16_t min_x = MIN(cursor.x, selection.x);
                    uint16_t min_y = MIN(cursor.y, selection.y);

                    selection.width = (cursor.x > selection.x) ? (cursor.x - selection.x + 1) : (selection.x - cursor.x + 1);
                    selection.height = (cursor.y > selection.y) ? (cursor.y - selection.y + 1) : (selection.y - cursor.y + 1);

                    selection.x = min_x;
                    selection.y = min_y;

                    int numpad = _menu_await_numpad();
                    _menu_copy_pattern(buffer_main, selection, numpad);
                    display_draw_cells(buffer_main);
                } else { // Selecting first point
                    selection.x = cursor.x;
                    selection.y = cursor.y;
                }

                select = !select;
                eadk_timing_msleep(menu_ms_delay); // Pause to avoid instant toggling
            } // end if (shift)

            // Display cursor and selection corners
            eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);

            if (select) {
                eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*SCALE, selection.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
                eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*SCALE, selection.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
                eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*SCALE, cursor.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
            }

            // Framerate in menu, resulting in speed of cursor. Switch to delta time please
            eadk_timing_msleep(menu_ms_delay/cursor_speed);

            // Clear cursor and selection corners
            eadk_display_push_rect_uniform(
                (eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE },
                (DIFF_COLOR) * buffer_main[ IDX( cursor.x, cursor.y ) ] + DEAD_COLOR
            );

            if (select) {
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ selection.x*SCALE, selection.y*SCALE, SCALE, SCALE },
                    (DIFF_COLOR) * buffer_main[ IDX( selection.x, selection.y ) ] + DEAD_COLOR
                );
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ cursor.x*SCALE, selection.y*SCALE, SCALE, SCALE },
                    (DIFF_COLOR) * buffer_main[ IDX( cursor.x, selection.y ) ] + DEAD_COLOR
                );
                eadk_display_push_rect_uniform(
                    (eadk_rect_t){ selection.x*SCALE, cursor.y*SCALE, SCALE, SCALE },
                    (DIFF_COLOR) * buffer_main[ IDX( selection.x, cursor.y ) ] + DEAD_COLOR
                );
            }
            // end if (pause)
        } else { // !pause, AKA run simulation
            step(buffer_main, buffer_alt);
            eadk_timing_msleep(FRAME_MS);
        }
    }

    return 0;
}
