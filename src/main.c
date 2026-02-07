#include "storage.h"
#include "font.h"
#include "config.h"
#include "conway.h"
#include "menu.h"
#include "macros.h"

#include <eadk.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Conway";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

// Declare config before macros
extern const char* CONFIG_FILE;

Config CONFIG;
int SCALE_IDX;
int SCALE;
int COLOR_IDX;
int FRAME_MS;
int STRICT_PASTE;
int FONT; // 0, 1 : Base, custom [font]


/* Constants and types */
typedef uint8_t cell_t;

const int MENU_MS_DELAY = 250;
const int AREA_MAX = 160 * 120;
// The calculator's memory, with no other files, can support around 6 patterns of
// size AREA_MAX, with worst RLE configuration (alternating pixels)

const eadk_color_t CURSOR_COLOR = 0xFCD5; // Pink
// Choose a color that uses all channels, or low SCALE values will
// make it hard to see where the cursor is aligned. Full red
// cursor appears to end higher than it should since red is
// the first channel

eadk_color_t CELL_COLOR = 0xFFFF;
eadk_color_t DEAD_COLOR = 0x0000;
eadk_color_t DIFF_COLOR = 0xFFFF;
char SAVE_FILE[] = "pattern0.cwp";
// Not const, as the 0 char gets reassigned.

const int scales[5] = { 1, 2, 4, 5, 8 };

const eadk_color_t cell_colors[3] = {
    0xFFFF, // White
    0xBECA, // Green
    0xFDCF, // Peach
};

const eadk_color_t dead_colors[3] = {
    0x0000, // Black
    0x4AA4, // Dark green
    0x6246, // Dark brown
};

const eadk_event_t numpad[10] = {
    48, 42, 43, 44, 36, 37, 38, 30, 31, 32 // Numpad events, 0-9
};


inline void clear_cursors(cell_t* buffer_main, eadk_point_t cursor, eadk_rect_t selection, bool select) {
    // Clear cursor and selection corners
    eadk_display_push_rect_uniform(
        (eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE },
        (DIFF_COLOR) * buffer_main[ IDX( cursor.x, cursor.y ) ] + DEAD_COLOR
    );

    if (!select) return;

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

inline void display_cursors(cell_t* buffer_main, eadk_point_t cursor, eadk_rect_t selection, bool select) {
    // Display cursor and selection corners
    eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);

    if (!select) return;

    eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*SCALE, selection.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
    eadk_display_push_rect_uniform((eadk_rect_t){ cursor.x*SCALE, selection.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
    eadk_display_push_rect_uniform((eadk_rect_t){ selection.x*SCALE, cursor.y*SCALE, SCALE, SCALE }, CURSOR_COLOR);
}

int main(int argc, char* argv[]) {
    // Load config
    CONFIG = load_config(CONFIG_FILE);
    SCALE_IDX = CONFIG.scale_idx;
    SCALE = scales[SCALE_IDX % 5];
    COLOR_IDX = CONFIG.color_idx;
    FRAME_MS = CONFIG.frame_ms;
    STRICT_PASTE = CONFIG.strict_paste;
    FONT = CONFIG.font;

    CELL_COLOR = cell_colors[COLOR_IDX];
    DEAD_COLOR = dead_colors[COLOR_IDX];
    DIFF_COLOR = CELL_COLOR - DEAD_COLOR;

    // Main "global" variables
    eadk_point_t cursor = { W/2, H/2 };

    // Selection tool for copy/paste
    eadk_rect_t selection = {0};
    uint8_t cursor_speed = 5;

    // First screen reset
    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);

    // Allocate buffers
    cell_t* buffer_main = calloc(H * W, sizeof(cell_t));
    cell_t* buffer_alt  = calloc(2 * W, sizeof(cell_t));

    // Should not happen ... but if it does, starting simulation will reset!
    if (buffer_main == NULL || buffer_alt == NULL) {
        eadk_display_draw_string(
            "alloc failed... quit app",
            (eadk_point_t){ 0, 0 },
            true, eadk_color_red, eadk_color_black
        );
        eadk_timing_msleep(5000);
    }

    // (Optional) Obsolete: load pattern from external_data (just use Upsilon Connector)
    // cells_insert_pattern(buffer_main, eadk_external_data, eadk_external_data_size, 0, 0);
    display_draw_cells(buffer_main);
    display_cursors(buffer_main, cursor, selection, false);

    // Menu flags
    bool pause = true;
    bool select = false;
    bool step_lock = false;
    bool panel_lock = false;
    bool menu_action = false;

    // Avoid app opening to cause keydown(OK)
    eadk_timing_msleep(MENU_MS_DELAY);

    while (true) {
        // Add option to toggle vblank ?
        // eadk_display_wait_for_vblank();
        eadk_keyboard_state_t kb = eadk_keyboard_scan();

        if (eadk_keyboard_key_down(kb, eadk_event_ok) && !select) {
            pause = !pause;
            if (!pause) clear_cursors(buffer_main, cursor, selection, false);
            eadk_timing_msleep(MENU_MS_DELAY); // Add delay to be usable
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

            if (!kb && !select) {
                if (menu_action) {
                    display_draw_cells(buffer_main);
                    display_cursors(buffer_main, cursor, selection, select);
                }
                menu_action = false;
                step_lock = false;
                panel_lock = false;
                continue;
            }

            menu_action = true;

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
            }

            if (sc != 0) {
                _menu_scale(sc);
            }

            // Copy entire screen
            if (eadk_keyboard_key_down(kb, eadk_event_multiplication)) {
                int numpad = _menu_await_numpad();
                bool status = _menu_copy_pattern(buffer_main, (eadk_rect_t){ 0, 0, W, H }, numpad);

                if (status) display_message("copied entire screen", 20);

            }

            // Change pasting mode Strict/Normal
            if (eadk_keyboard_key_down(kb, eadk_event_division)) {
                STRICT_PASTE = !STRICT_PASTE;
                display_message(STRICT_PASTE ? "pasting: strict mode" : "pasting: transparent", 20);
            }

            // Pasting pattern from memory
            if (eadk_keyboard_key_down(kb, eadk_event_ans)) {
                int numpad = _menu_await_numpad();
                _menu_paste_pattern(buffer_main, cursor, numpad);
            }

            // Save config
            if (eadk_keyboard_key_down(kb, eadk_event_exe)) {
                _menu_save_config();
            }

            // Handle stepping
            if (eadk_keyboard_key_down(kb, eadk_event_back) && !step_lock) {
                step(buffer_main, buffer_alt);
                // No wait since key has to be released
            }

            step_lock = eadk_keyboard_key_down(kb, eadk_event_back);

            // Handle panel display
            if (eadk_keyboard_key_down(kb, eadk_event_var) && !panel_lock) {
                // @fixme
                //_menu_panel();
                ;
            }

            panel_lock = eadk_keyboard_key_down(kb, eadk_event_var);

            // Handle font toggle
            if (eadk_keyboard_key_down(kb, eadk_event_ln)) {
                FONT = !FONT;
                display_message(FONT ? "font: pixel" : "font: basic", 11);
            }

            // if (!x && !y) continue;

            clear_cursors(buffer_main, cursor, selection, select);

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
                eadk_timing_msleep(MENU_MS_DELAY); // Pause to avoid instant toggling
            } // end if (shift)

            display_cursors(buffer_main, cursor, selection, select);

            // Framerate in menu, resulting in speed of cursor. Switch to delta time please
            eadk_timing_msleep(MENU_MS_DELAY/cursor_speed);
            // end if (pause)
        } else { // !pause, AKA run simulation
            step(buffer_main, buffer_alt);
            eadk_timing_msleep(FRAME_MS);
        }
    }

    return 0;
}

