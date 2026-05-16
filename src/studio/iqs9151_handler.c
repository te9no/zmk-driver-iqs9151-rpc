#include <pb_decode.h>
#include <pb_encode.h>
#include <stdio.h>
#include <zmk/iqs9151/iqs9151.pb.h>
#include <zmk/iqs9151_runtime.h>
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
    config.two_finger_tap = runtime->two_finger_tap;
    config.three_finger_tap = runtime->three_finger_tap;
    config.scroll_x = runtime->scroll_x;
    config.scroll_y = runtime->scroll_y;
    config.pinch = runtime->pinch;
    config.cursor_inertia = runtime->cursor_inertia;
    config.scroll_inertia = runtime->scroll_inertia;

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
        .two_finger_tap = config->two_finger_tap,
        .three_finger_tap = config->three_finger_tap,
        .scroll_x = config->scroll_x,
        .scroll_y = config->scroll_y,
        .pinch = config->pinch,
        .cursor_inertia = config->cursor_inertia,
        .scroll_inertia = config->scroll_inertia,
    };
}

static bool proto_config_is_valid(const zmk_iqs9151_Iqs9151Config *config) {
    return config->resolution_x <= 4095U &&
           config->resolution_y <= 4095U &&
           config->ati_target_count <= 1000U &&
           config->dynamic_filter_bottom_speed <= 2047U &&
           config->dynamic_filter_top_speed <= 2047U &&
           config->dynamic_filter_bottom_beta <= 255U;
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

    config.config = runtime_to_proto(iqs9151_runtime_config_get());
    resp->which_response_type = zmk_iqs9151_Response_config_tag;
    resp->response_type.config = config;
#else
    set_error_response(resp, "IQS9151 device is not available in this firmware image");
#endif
}

static void handle_set_config_request(const zmk_iqs9151_SetConfigRequest *req,
                                      zmk_iqs9151_Response *resp) {
#if IS_ENABLED(CONFIG_INPUT_IQS9151)
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
    set_config.config = runtime_to_proto(iqs9151_runtime_config_get());
    resp->which_response_type = zmk_iqs9151_Response_set_config_tag;
    resp->response_type.set_config = set_config;
#else
    ARG_UNUSED(req);
    set_error_response(resp, "IQS9151 device is not available in this firmware image");
#endif
}

static void handle_reset_config_request(zmk_iqs9151_Response *resp) {
#if IS_ENABLED(CONFIG_INPUT_IQS9151)
    iqs9151_runtime_config_reset();

    zmk_iqs9151_ResetConfigResponse reset_config = zmk_iqs9151_ResetConfigResponse_init_zero;
    reset_config.config = runtime_to_proto(iqs9151_runtime_config_get());
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
