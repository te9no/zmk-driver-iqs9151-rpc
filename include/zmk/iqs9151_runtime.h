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
    bool one_finger_presshold;
    uint16_t one_finger_tap_max_ms;
    uint16_t one_finger_tap_move;
    uint16_t one_finger_tapdrag_gap_max_ms;
    bool two_finger_tap;
    bool two_finger_presshold;
    uint16_t two_finger_tap_max_ms;
    uint16_t two_finger_tap_move;
    uint16_t two_finger_tapdrag_gap_max_ms;
    bool three_finger_tap;
    bool three_finger_presshold;
    uint16_t three_finger_tap_max_ms;
    uint16_t three_finger_tap_move;
    uint16_t three_finger_tapdrag_gap_max_ms;
    uint16_t three_finger_swipe_threshold;
    bool scroll_x;
    bool scroll_y;
    uint16_t two_finger_scroll_start_move;
    bool pinch;
    uint16_t two_finger_pinch_start_distance;
    uint8_t two_finger_pinch_wheel_gain_x10;
    bool cursor_inertia;
    uint16_t cursor_inertia_decay;
    uint16_t cursor_inertia_recent_window_ms;
    uint16_t cursor_inertia_stale_gap_ms;
    uint8_t cursor_inertia_min_samples;
    uint16_t cursor_inertia_min_avg_speed;
    bool scroll_inertia;
    uint16_t scroll_inertia_decay;
    uint16_t scroll_inertia_recent_window_ms;
    uint16_t scroll_inertia_stale_gap_ms;
    uint8_t scroll_inertia_min_samples;
    uint16_t scroll_inertia_min_avg_speed;
};

const struct iqs9151_runtime_config *iqs9151_runtime_config_get(void);
int iqs9151_runtime_config_set(const struct iqs9151_runtime_config *config);
void iqs9151_runtime_config_reset(void);
