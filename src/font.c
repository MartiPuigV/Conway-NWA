#include <eadk.h>
#include <stdlib.h>
#include <stdint.h>

typedef uint16_t Glyph;

const int GLYPH_SPACING = 1;

const Glyph charset[] = {
    0x7DFD, 0xEBDF, 0x7CCF, 0xEDDE, 0xFECF,
    0xFECC, 0x7CDF, 0xDFFD, 0xF66F, 0xF3BF,
    0xDEED, 0xCCCF, 0xFB99, 0xDDBB, 0x7DDF,
    0xFDFC, 0x6AAF, 0xFDED, 0xFC3F, 0xF666,
    0xDDD7, 0x99b6, 0x99BF, 0xDE7B, 0xDF66,
    0xF3CF, 0x7DDF, 0xE66F, 0xF3EF, 0xF13F,
    0xDDF1, 0xFC7F, 0x8F9F, 0xF366, 0x7DBE,
    0xF9F1, 0x0002, 0x2200, 0x5500, 0xF302,
    0x6606, 0xC88C, 0x3113, 0x04E4, 0x00E0,
    0x2020, 0x8226, 0x0000,
};

const char charmap[] = "abcdefghijklmnopqrstuvwxyz0123456789.'\"?![]+-:; ";

void display_glyph(Glyph glyph, eadk_point_t pos, int scale, eadk_color_t fore, eadk_color_t back) {
    if (!glyph) return;

    eadk_color_t diff = fore - back;

    for (size_t i = 0; i < 16; i++) {
        eadk_display_push_rect_uniform(
            (eadk_rect_t){ pos.x + scale * (i % 4), pos.y + scale * (i / 4), scale, scale },
            diff * (glyph >> (15 - i) & 1) + back
        );
    }
}

Glyph charset_lookup(char c) {
    for (size_t i = 0; i < 48; i++) {
        if (charmap[i] == c) return charset[i];
    }

    return charset[0];
}

void display_string(const char* string, size_t len, eadk_point_t pos, int scale, eadk_color_t fore, eadk_color_t back) {
    for (size_t i = 0; i < len; i++) {
        char c = string[i];
        Glyph glyph = charset_lookup(c);
        display_glyph(glyph, pos, scale, fore, back);
        pos.x += (4 + GLYPH_SPACING) * scale;
    }
}

