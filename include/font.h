#include <eadk.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef FONT_H_
#define FONT_H_

typedef uint16_t Glyph;

void display_glyph(Glyph glyph, eadk_point_t pos, int scale, eadk_color_t fore, eadk_color_t back);

Glyph charset_lookup(char c);

void display_string(const char* string, size_t len, eadk_point_t pos, int scale, eadk_color_t fore, eadk_color_t back);

#endif // FONT_H_

