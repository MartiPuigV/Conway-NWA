#include <stdlib.h>

#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct {
    int scale_idx;
    int color_idx;
    int frame_ms;
    int strict_paste;
    int font;
} Config;

Config load_config(const char* config_file);

void save_config(const char* config);

#endif // CONFIG_H_

