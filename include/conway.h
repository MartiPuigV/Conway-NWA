#include "macros.h"

#include <eadk.h>
#include <stdlib.h>

#ifndef CONWAY_H_
#define CONWAY_H_

typedef uint8_t cell_t;

uint8_t get_neighbors_wrap(const cell_t* cells, cell_t* top_row, size_t x, size_t y);

uint8_t get_neighbors_no_wrap(const cell_t* cells, cell_t* top_row, size_t x, size_t y);

void step(cell_t* restrict buffer_main, cell_t* restrict buffer_alt);

void display_draw_cells(const cell_t* cells);

void cells_insert_pattern(cell_t* buffer_main, const char* pattern, size_t size, eadk_point_t pos);

char* rle_encode(const cell_t* cells, size_t size, size_t* out_size);

char* yank_pattern(const cell_t* buffer_main, size_t* out_size, eadk_rect_t area);

#endif // CONWAY_H_

