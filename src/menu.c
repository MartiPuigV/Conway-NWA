#include "macros.h"
#include "storage.h"
#include "config.h"
#include "conway.h"
#include "font.h"

#include <eadk.h>
#include <stdlib.h>

typedef uint8_t cell_t;

extern int COLOR_IDX, SCALE_IDX, SCALE, FRAME_MS, FONT, STRICT_PASTE, FONT;
extern const int AREA_MAX, MENU_MS_DELAY;
extern eadk_color_t CELL_COLOR, DEAD_COLOR, DIFF_COLOR;
extern char SAVE_FILE[];

extern const int scales[5];
extern const eadk_color_t cell_colors[3];
extern const eadk_color_t dead_colors[3];
extern const eadk_event_t numpad[10];

const int eadk_event_to_numpad(eadk_event_t event) {
    // Translates the event ID to [0-9] if event is a numpad key else -1
    for (size_t i = 0; i < 10; i++) {
        if (numpad[i] == event) return i;
    }

    return -1;
}

void display_message(const char* message, size_t len) {
    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);
    const int scale = 2;

    if (FONT) {
        eadk_point_t pos = { EADK_SCREEN_WIDTH/2 - (2*scale + 1)*len, EADK_SCREEN_HEIGHT/2 - 2*scale };
        display_string(message, len, pos, scale, CELL_COLOR, DEAD_COLOR);
    } else {
        eadk_point_t pos = { EADK_SCREEN_WIDTH/2 - 5*len, EADK_SCREEN_HEIGHT/2 - 10 };
        eadk_display_draw_string(message, pos, true, CELL_COLOR, DEAD_COLOR);
    }

    eadk_timing_msleep(MENU_MS_DELAY);
}

void _menu_color(cell_t* buffer_main) {
    COLOR_IDX = (COLOR_IDX + 1) % 3;
    CELL_COLOR = cell_colors[COLOR_IDX];
    DEAD_COLOR = dead_colors[COLOR_IDX];
    DIFF_COLOR = CELL_COLOR - DEAD_COLOR;
    display_draw_cells(buffer_main);
    eadk_timing_msleep(MENU_MS_DELAY);
}

void _menu_mod(cell_t* buffer, eadk_point_t cursor, int mod) {
    eadk_color_t c = (mod == 1) ? 1 : 0; // Turns (-1, 1) into (0, 1)
    buffer[IDX(cursor.x, cursor.y)] = c;
    eadk_display_push_rect_uniform(
        (eadk_rect_t){ cursor.x*SCALE, cursor.y*SCALE, SCALE, SCALE },
        (DIFF_COLOR) * c + DEAD_COLOR
    );
}

void _menu_paste_pattern(cell_t* buffer, eadk_point_t cursor, int savefile_idx) {
    size_t pattern_size = 0;
    SAVE_FILE[7] = '0'+savefile_idx;

    if (!extapp_fileExists(SAVE_FILE)) {
        display_message("that savefile doesn't exist!", 28);
        display_draw_cells(buffer);
        return;
    }

    const char* pattern = extapp_fileRead(SAVE_FILE, &pattern_size);

    cells_insert_pattern(buffer, pattern, pattern_size, cursor); // Insert pattern into array
    display_draw_cells(buffer); // Update display with newly pasted cells
    eadk_timing_msleep(MENU_MS_DELAY);
}

void _menu_save_config() {
    const char config[5] = { (char)SCALE_IDX, (char)COLOR_IDX, (char)FRAME_MS, (char)STRICT_PASTE, (char)FONT };
    save_config(config);
    display_message("saved config", 12);
}

bool _menu_copy_pattern(const cell_t* buffer, eadk_rect_t area, int savefile_idx) {
    if (area.width * area.height > AREA_MAX) {
        display_message("area too large!", 15);
        return false;
    }

    size_t yank_size = 0;
    char* selected_cells = yank_pattern(buffer, &yank_size, area);
    SAVE_FILE[7] = '0'+savefile_idx;

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

void _menu_scale(int sc) {
    SCALE_IDX = (SCALE_IDX + 5 + sc) % 5;

    char msg[] = "[,] scale:  ";
    msg[1] -= sc;
    // Some magic ASCII. Offset ',' by -sc.
    // + : 43
    // , : 44
    // - : 45
    msg[11] = '0' + scales[SCALE_IDX];
    // A classic

    display_message(msg, 12);
}

void _menu_ms(int ms) {
    FRAME_MS = MIN(MAX(0, FRAME_MS + 10*ms), 250); // Clamp in [0, 250] range (min. of 4 FPS)

    char msg[] = "[,] 000 ms.frame";
    msg[1] -= ms; // Same magic as above
    msg[4] = (FRAME_MS / 100)      + '0';
    msg[5] = (FRAME_MS / 10 ) % 10 + '0';
    msg[6] = (FRAME_MS % 10 )      + '0';

    display_message(msg, 16);
}

int _menu_await_numpad() {
    int32_t timeout;
    eadk_event_t key;
    int numpad;

    eadk_display_push_rect_uniform(eadk_screen_rect, DEAD_COLOR);
    display_message("select number [0-9]", 19);

    while (true) {
        key = eadk_event_get(&timeout);
        numpad = eadk_event_to_numpad(key);
        if (numpad != -1) return numpad;
    }
}

