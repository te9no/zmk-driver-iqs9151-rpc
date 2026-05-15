#pragma once

#include <stdbool.h>
#include <stdint.h>

struct iqs9151_runtime_config {
    uint16_t resolution_x;
    uint16_t resolution_y;
    uint16_t ati_target_count;
    uint16_t dynamic_filter_bottom_speed;
    uint16_t dynamic_filter_top_speed;
    uint8_t dynamic_filter_bottom_beta;
    bool one_finger_tap;
    bool two_finger_tap;
    bool three_finger_tap;
    bool scroll_x;
    bool scroll_y;
    bool pinch;
    bool cursor_inertia;
    bool scroll_inertia;
};

const struct iqs9151_runtime_config *iqs9151_runtime_config_get(void);
int iqs9151_runtime_config_set(const struct iqs9151_runtime_config *config);
void iqs9151_runtime_config_reset(void);
