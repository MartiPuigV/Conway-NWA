#include "macros.h"
#include "storage.h"
#include "config.h"

#include <eadk.h>
#include <stdlib.h>

#ifndef MENU_H_
#define MENU_H_

typedef uint8_t cell_t;

extern int COLOR_IDX, SCALE_IDX, SCALE, FRAME_MS, STRICT_PASTE, FONT;
extern const int AREA_MAX, MENU_MS_DELAY;
extern eadk_color_t CELL_COLOR, DEAD_COLO, DIFF_COLOR;
extern char SAVEFILE[];

const int eadk_event_to_numpad(eadk_event_t event);

void display_message(const char* message, size_t len);

void _menu_color(cell_t* buffer_main);

void _menu_mod(cell_t* buffer, eadk_point_t cursor, int mod);

void _menu_paste_pattern(cell_t* buffer, eadk_point_t cursor, int savefile_idx);

void _menu_save_config();

bool _menu_copy_pattern(const cell_t* buffer, eadk_rect_t area, int savefile_idx);

void _menu_scale(int sc);

void _menu_ms(int ms);

int _menu_await_numpad();

#endif // MENU_H_

