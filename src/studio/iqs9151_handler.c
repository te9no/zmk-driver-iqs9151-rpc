#include <pb_decode.h>
#include <pb_encode.h>
#include <stdio.h>
#include <zmk/iqs9151/iqs9151.pb.h>
#include <zmk/iqs9151_runtime.h>
#include <zmk/iqs9151_split.h>
#include <zmk/studio/custom.h>

#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_rpc_custom_subsystem_meta iqs9151_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS("https://te9no.github.io/zmk-driver-iqs9151-rpc/"),
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

ZMK_RPC_CUSTOM_SUBSYSTEM(zmk__iqs9151, &iqs9151_meta, iqs9151_rpc_handle_request);
ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(zmk__iqs9151, zmk_iqs9151_Response);
ZMK_RPC_CUSTOM_SUBSYSTEM(dya__iqs9151, &iqs9151_meta, iqs9151_rpc_handle_request);

static void set_error_response(zmk_iqs9151_Response *resp, const char *message) {
    zmk_iqs9151_ErrorResponse err = zmk_iqs9151_ErrorResponse_init_zero;

    snprintf(err.message, sizeof(err.message), "%s", message);
    resp->which_response_type = zmk_iqs9151_Response_error_tag;
    resp->response_type.error = err;
}

static const char *split_error_message(int ret, const char *fallback) {
    switch (ret) {
    case -ETIMEDOUT:
        return "split IQS9151 request timed out";
    case -EMSGSIZE:
        return "split IQS9151 firmware mismatch; flash both halves with the latest firmware";
    default:
        return fallback;
    }
}

static zmk_iqs9151_Iqs9151Config
runtime_to_proto(const struct iqs9151_runtime_config *runtime) {
    zmk_iqs9151_Iqs9151Config config = zmk_iqs9151_Iqs9151Config_init_zero;

    config.resolution_x = runtime->resolution_x;
    config.resolution_y = runtime->resolution_y;
    config.ati_target_count = runtime->ati_target_count;
    config.dynamic_filter_bottom_speed = runtime->dynamic_filter_bottom_speed;
    config.dynamic_filter_top_speed = runtime->dynamic_filter_top_speed;
    config.dynamic_filter_bottom_beta = runtime->dynamic_filter_bottom_beta;
    config.one_finger_tap = runtime->one_finger_tap;
    config.one_finger_presshold = runtime->one_finger_presshold;
    config.one_finger_tap_max_ms = runtime->one_finger_tap_max_ms;
    config.one_finger_tap_move = runtime->one_finger_tap_move;
    config.one_finger_tapdrag_gap_max_ms = runtime->one_finger_tapdrag_gap_max_ms;
    config.two_finger_tap = runtime->two_finger_tap;
    config.two_finger_presshold = runtime->two_finger_presshold;
    config.two_finger_tap_max_ms = runtime->two_finger_tap_max_ms;
    config.two_finger_tap_move = runtime->two_finger_tap_move;
    config.two_finger_tapdrag_gap_max_ms = runtime->two_finger_tapdrag_gap_max_ms;
    config.three_finger_tap = runtime->three_finger_tap;
    config.three_finger_presshold = runtime->three_finger_presshold;
    config.three_finger_tap_max_ms = runtime->three_finger_tap_max_ms;
    config.three_finger_tap_move = runtime->three_finger_tap_move;
    config.three_finger_tapdrag_gap_max_ms = runtime->three_finger_tapdrag_gap_max_ms;
    config.three_finger_swipe_threshold = runtime->three_finger_swipe_threshold;
    config.scroll_x = runtime->scroll_x;
    config.scroll_y = runtime->scroll_y;
    config.two_finger_scroll_start_move = runtime->two_finger_scroll_start_move;
    config.scroll_speed_x10 = runtime->scroll_speed_x10;
    config.pinch = runtime->pinch;
    config.two_finger_pinch_start_distance = runtime->two_finger_pinch_start_distance;
    config.two_finger_pinch_wheel_gain_x10 = runtime->two_finger_pinch_wheel_gain_x10;
    config.cursor_speed_x10 = runtime->cursor_speed_x10;
    config.cursor_inertia = runtime->cursor_inertia;
    config.cursor_inertia_decay = runtime->cursor_inertia_decay;
    config.cursor_inertia_recent_window_ms = runtime->cursor_inertia_recent_window_ms;
    config.cursor_inertia_stale_gap_ms = runtime->cursor_inertia_stale_gap_ms;
    config.cursor_inertia_min_samples = runtime->cursor_inertia_min_samples;
    config.cursor_inertia_min_avg_speed = runtime->cursor_inertia_min_avg_speed;
    config.scroll_inertia = runtime->scroll_inertia;
    config.scroll_inertia_decay = runtime->scroll_inertia_decay;
    config.scroll_inertia_recent_window_ms = runtime->scroll_inertia_recent_window_ms;
    config.scroll_inertia_stale_gap_ms = runtime->scroll_inertia_stale_gap_ms;
    config.scroll_inertia_min_samples = runtime->scroll_inertia_min_samples;
    config.scroll_inertia_min_avg_speed = runtime->scroll_inertia_min_avg_speed;

    return config;
}

static struct iqs9151_runtime_config
proto_to_runtime(const zmk_iqs9151_Iqs9151Config *config) {
    return (struct iqs9151_runtime_config){
        .resolution_x = config->resolution_x,
        .resolution_y = config->resolution_y,
        .ati_target_count = config->ati_target_count,
        .dynamic_filter_bottom_speed = config->dynamic_filter_bottom_speed,
        .dynamic_filter_top_speed = config->dynamic_filter_top_speed,
        .dynamic_filter_bottom_beta = config->dynamic_filter_bottom_beta,
        .one_finger_tap = config->one_finger_tap,
        .one_finger_presshold = config->one_finger_presshold,
        .one_finger_tap_max_ms = config->one_finger_tap_max_ms,
        .one_finger_tap_move = config->one_finger_tap_move,
        .one_finger_tapdrag_gap_max_ms = config->one_finger_tapdrag_gap_max_ms,
        .two_finger_tap = config->two_finger_tap,
        .two_finger_presshold = config->two_finger_presshold,
        .two_finger_tap_max_ms = config->two_finger_tap_max_ms,
        .two_finger_tap_move = config->two_finger_tap_move,
        .two_finger_tapdrag_gap_max_ms = config->two_finger_tapdrag_gap_max_ms,
        .three_finger_tap = config->three_finger_tap,
        .three_finger_presshold = config->three_finger_presshold,
        .three_finger_tap_max_ms = config->three_finger_tap_max_ms,
        .three_finger_tap_move = config->three_finger_tap_move,
        .three_finger_tapdrag_gap_max_ms = config->three_finger_tapdrag_gap_max_ms,
        .three_finger_swipe_threshold = config->three_finger_swipe_threshold,
        .scroll_x = config->scroll_x,
        .scroll_y = config->scroll_y,
        .two_finger_scroll_start_move = config->two_finger_scroll_start_move,
        .scroll_speed_x10 = config->scroll_speed_x10,
        .pinch = config->pinch,
        .two_finger_pinch_start_distance = config->two_finger_pinch_start_distance,
        .two_finger_pinch_wheel_gain_x10 = config->two_finger_pinch_wheel_gain_x10,
        .cursor_speed_x10 = config->cursor_speed_x10,
        .cursor_inertia = config->cursor_inertia,
        .cursor_inertia_decay = config->cursor_inertia_decay,
        .cursor_inertia_recent_window_ms = config->cursor_inertia_recent_window_ms,
        .cursor_inertia_stale_gap_ms = config->cursor_inertia_stale_gap_ms,
        .cursor_inertia_min_samples = config->cursor_inertia_min_samples,
        .cursor_inertia_min_avg_speed = config->cursor_inertia_min_avg_speed,
        .scroll_inertia = config->scroll_inertia,
        .scroll_inertia_decay = config->scroll_inertia_decay,
        .scroll_inertia_recent_window_ms = config->scroll_inertia_recent_window_ms,
        .scroll_inertia_stale_gap_ms = config->scroll_inertia_stale_gap_ms,
        .scroll_inertia_min_samples = config->scroll_inertia_min_samples,
        .scroll_inertia_min_avg_speed = config->scroll_inertia_min_avg_speed,
    };
}

static bool proto_config_is_valid(const zmk_iqs9151_Iqs9151Config *config) {
    return config->resolution_x <= 4095U &&
           config->resolution_y <= 4095U &&
           config->ati_target_count <= 1000U &&
           config->dynamic_filter_bottom_speed <= 2047U &&
           config->dynamic_filter_top_speed <= 2047U &&
           config->dynamic_filter_bottom_beta <= 255U &&
           config->one_finger_tap_max_ms >= 1U &&
           config->one_finger_tap_max_ms <= 1000U &&
           config->one_finger_tap_move >= 1U &&
           config->one_finger_tap_move <= 1000U &&
           config->one_finger_tapdrag_gap_max_ms >= 1U &&
           config->one_finger_tapdrag_gap_max_ms <= 1000U &&
           config->two_finger_tap_max_ms >= 1U &&
           config->two_finger_tap_max_ms <= 1000U &&
           config->two_finger_tap_move >= 1U &&
           config->two_finger_tap_move <= 1000U &&
           config->two_finger_tapdrag_gap_max_ms >= 1U &&
           config->two_finger_tapdrag_gap_max_ms <= 1000U &&
           config->three_finger_tap_max_ms >= 1U &&
           config->three_finger_tap_max_ms <= 1000U &&
           config->three_finger_tap_move >= 1U &&
           config->three_finger_tap_move <= 1000U &&
           config->three_finger_tapdrag_gap_max_ms >= 1U &&
           config->three_finger_tapdrag_gap_max_ms <= 1000U &&
           config->three_finger_swipe_threshold <= 1000U &&
           config->two_finger_scroll_start_move >= 1U &&
           config->two_finger_scroll_start_move <= 2000U &&
           config->scroll_speed_x10 >= 1U &&
           config->scroll_speed_x10 <= 100U &&
           config->two_finger_pinch_start_distance >= 1U &&
           config->two_finger_pinch_start_distance <= 2000U &&
           config->two_finger_pinch_wheel_gain_x10 >= 1U &&
           config->two_finger_pinch_wheel_gain_x10 <= 100U &&
           config->cursor_speed_x10 >= 1U &&
           config->cursor_speed_x10 <= 100U &&
           config->cursor_inertia_decay <= 1000U &&
           config->cursor_inertia_recent_window_ms >= 1U &&
           config->cursor_inertia_recent_window_ms <= 500U &&
           config->cursor_inertia_stale_gap_ms >= 1U &&
           config->cursor_inertia_stale_gap_ms <= 500U &&
           config->cursor_inertia_min_samples >= 1U &&
           config->cursor_inertia_min_samples <= 12U &&
           config->cursor_inertia_min_avg_speed >= 1U &&
           config->cursor_inertia_min_avg_speed <= 500U &&
           config->scroll_inertia_decay <= 1000U &&
           config->scroll_inertia_recent_window_ms >= 1U &&
           config->scroll_inertia_recent_window_ms <= 500U &&
           config->scroll_inertia_stale_gap_ms >= 1U &&
           config->scroll_inertia_stale_gap_ms <= 500U &&
           config->scroll_inertia_min_samples >= 1U &&
           config->scroll_inertia_min_samples <= 12U &&
           config->scroll_inertia_min_avg_speed >= 1U &&
           config->scroll_inertia_min_avg_speed <= 500U;
}

static void handle_ping_request(const zmk_iqs9151_PingRequest *req,
                                zmk_iqs9151_Response *resp) {
    zmk_iqs9151_PingResponse ping = zmk_iqs9151_PingResponse_init_zero;

    snprintf(ping.message, sizeof(ping.message), "iqs9151:%u", req->value);
    resp->which_response_type = zmk_iqs9151_Response_ping_tag;
    resp->response_type.ping = ping;
}

static void handle_get_config_request(zmk_iqs9151_Response *resp) {
#if IS_ENABLED(CONFIG_INPUT_IQS9151)
    zmk_iqs9151_ConfigResponse config = zmk_iqs9151_ConfigResponse_init_zero;

    config.has_config = true;
    config.config = runtime_to_proto(iqs9151_runtime_config_get());
    resp->which_response_type = zmk_iqs9151_Response_config_tag;
    resp->response_type.config = config;
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct iqs9151_runtime_config runtime;
    int ret = iqs9151_split_get_config(&runtime);

    if (ret != 0) {
        LOG_WRN("Failed to read split IQS9151 config: %d", ret);
        set_error_response(resp, split_error_message(ret, "failed to read split IQS9151 config"));
        return;
    }

    zmk_iqs9151_ConfigResponse config = zmk_iqs9151_ConfigResponse_init_zero;
    config.has_config = true;
    config.config = runtime_to_proto(&runtime);
    resp->which_response_type = zmk_iqs9151_Response_config_tag;
    resp->response_type.config = config;
#else
    set_error_response(resp, "IQS9151 device is not available in this firmware image");
#endif
}

static void handle_set_config_request(const zmk_iqs9151_SetConfigRequest *req,
                                      zmk_iqs9151_Response *resp) {
#if IS_ENABLED(CONFIG_INPUT_IQS9151)
    if (!req->has_config) {
        set_error_response(resp, "missing config");
        return;
    }
    if (!proto_config_is_valid(&req->config)) {
        set_error_response(resp, "invalid config");
        return;
    }

    struct iqs9151_runtime_config config = proto_to_runtime(&req->config);
    int ret = iqs9151_runtime_config_set(&config);

    if (ret != 0) {
        LOG_WRN("Failed to apply IQS9151 config: %d", ret);
        set_error_response(resp, ret == -EINVAL ? "invalid config" : "failed to apply config");
        return;
    }

    zmk_iqs9151_SetConfigResponse set_config = zmk_iqs9151_SetConfigResponse_init_zero;
    set_config.has_config = true;
    set_config.config = runtime_to_proto(iqs9151_runtime_config_get());
    resp->which_response_type = zmk_iqs9151_Response_set_config_tag;
    resp->response_type.set_config = set_config;
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (!req->has_config) {
        set_error_response(resp, "missing config");
        return;
    }
    if (!proto_config_is_valid(&req->config)) {
        set_error_response(resp, "invalid config");
        return;
    }

    struct iqs9151_runtime_config config = proto_to_runtime(&req->config);
    struct iqs9151_runtime_config applied;
    int ret = iqs9151_split_set_config(&config, &applied);

    if (ret != 0) {
        LOG_WRN("Failed to apply split IQS9151 config: %d", ret);
        set_error_response(resp, split_error_message(ret, "failed to apply split IQS9151 config"));
        return;
    }

    zmk_iqs9151_SetConfigResponse set_config = zmk_iqs9151_SetConfigResponse_init_zero;
    set_config.has_config = true;
    set_config.config = runtime_to_proto(&applied);
    resp->which_response_type = zmk_iqs9151_Response_set_config_tag;
    resp->response_type.set_config = set_config;
#else
    ARG_UNUSED(req);
    set_error_response(resp, "IQS9151 device is not available in this firmware image");
#endif
}

static void handle_reset_config_request(zmk_iqs9151_Response *resp) {
#if IS_ENABLED(CONFIG_INPUT_IQS9151)
    int ret = iqs9151_runtime_config_reset();
    if (ret != 0) {
        LOG_WRN("Failed to reset IQS9151 config: %d", ret);
        set_error_response(resp, "failed to reset config");
        return;
    }

    zmk_iqs9151_ResetConfigResponse reset_config = zmk_iqs9151_ResetConfigResponse_init_zero;
    reset_config.has_config = true;
    reset_config.config = runtime_to_proto(iqs9151_runtime_config_get());
    resp->which_response_type = zmk_iqs9151_Response_reset_config_tag;
    resp->response_type.reset_config = reset_config;
#elif IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct iqs9151_runtime_config runtime;
    int ret = iqs9151_split_reset_config(&runtime);

    if (ret != 0) {
        LOG_WRN("Failed to reset split IQS9151 config: %d", ret);
        set_error_response(resp, split_error_message(ret, "failed to reset split IQS9151 config"));
        return;
    }

    zmk_iqs9151_ResetConfigResponse reset_config = zmk_iqs9151_ResetConfigResponse_init_zero;
    reset_config.has_config = true;
    reset_config.config = runtime_to_proto(&runtime);
    resp->which_response_type = zmk_iqs9151_Response_reset_config_tag;
    resp->response_type.reset_config = reset_config;
#else
    set_error_response(resp, "IQS9151 device is not available in this firmware image");
#endif
}

static bool iqs9151_rpc_handle_request(const zmk_custom_CallRequest *raw_request,
                                       pb_callback_t *encode_response) {
    zmk_iqs9151_Response *resp =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(zmk__iqs9151, encode_response);
    zmk_iqs9151_Request req = zmk_iqs9151_Request_init_zero;
    pb_istream_t req_stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);

    if (!pb_decode(&req_stream, zmk_iqs9151_Request_fields, &req)) {
        LOG_WRN("Failed to decode IQS9151 RPC request: %s", PB_GET_ERROR(&req_stream));
        set_error_response(resp, "failed to decode request");
        return true;
    }

    switch (req.which_request_type) {
    case zmk_iqs9151_Request_ping_tag:
        handle_ping_request(&req.request_type.ping, resp);
        break;
    case zmk_iqs9151_Request_get_config_tag:
        handle_get_config_request(resp);
        break;
    case zmk_iqs9151_Request_set_config_tag:
        handle_set_config_request(&req.request_type.set_config, resp);
        break;
    case zmk_iqs9151_Request_reset_config_tag:
        handle_reset_config_request(resp);
        break;
    default:
        LOG_WRN("Unsupported IQS9151 RPC request type: %d", req.which_request_type);
        set_error_response(resp, "unsupported request");
        break;
    }

    return true;
}
