#include "storage.h"
#include <stdlib.h>

typedef struct {
    int scale_idx;
    int color_idx;
    int frame_ms;
    int strict_paste;
    int font;
} Config;

const char* CONFIG_FILE = "conway.conf";

Config load_config(const char* config_file) {
    Config default_cfg = (Config){ 2, 0, 50, 1, 0 };

    if (extapp_fileExists(config_file)) {
        size_t size = 0;
        const char* raw = extapp_fileRead(config_file, &size);

        if (size < 5) {
            // Should contain at least 5 bytes
            return default_cfg;
        }

        return (Config){ raw[0], raw[1], raw[2], raw[3], raw[4] };
    }

    return default_cfg;
}

void save_config(const char* config) {
    if (extapp_fileExists(CONFIG_FILE)) extapp_fileErase(CONFIG_FILE);
    extapp_fileWrite(CONFIG_FILE, config, 5);
}

