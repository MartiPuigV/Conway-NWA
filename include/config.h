#include <stdlib.h>
#include <stdint.h>

#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct {
    uint8_t scale_idx;
    uint8_t color_idx;
    uint8_t frame_ms;
    uint8_t strict_paste;
    uint8_t font;
    uint8_t wrap;
} Config;

Config load_config(const char* config_file);

void save_config(const char* config);

#endif // CONFIG_H_

