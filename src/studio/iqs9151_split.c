#include <zmk/event_manager.h>
#include <zmk/iqs9151_runtime.h>
#include <zmk/iqs9151_split.h>
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum iqs9151_split_request_type {
    IQS9151_SPLIT_REQUEST_GET = 1,
    IQS9151_SPLIT_REQUEST_SET = 2,
    IQS9151_SPLIT_REQUEST_RESET = 3,
};

#define IQS9151_SPLIT_DATA_SIZE 8
#define IQS9151_SPLIT_CHUNK_DONE 0x80
#define IQS9151_SPLIT_TIMEOUT_MS 5000

struct iqs9151_split_request {
    uint8_t source;
    uint8_t seq;
    uint8_t type;
    uint8_t chunk;
    uint8_t total_len;
    uint8_t data[IQS9151_SPLIT_DATA_SIZE];
};

struct iqs9151_split_response {
    uint8_t source;
    uint8_t seq;
    int8_t status;
    uint8_t chunk;
    uint8_t total_len;
    uint8_t data[IQS9151_SPLIT_DATA_SIZE];
};

ZMK_EVENT_DECLARE(iqs9151_split_request);
ZMK_EVENT_DECLARE(iqs9151_split_response);
ZMK_EVENT_IMPL(iqs9151_split_request);
ZMK_EVENT_IMPL(iqs9151_split_response);

ZMK_RELAY_EVENT_CENTRAL_TO_PERIPHERAL(iqs9151_split_request, iq, source);
ZMK_RELAY_EVENT_HANDLE(iqs9151_split_request, iq, source);
ZMK_RELAY_EVENT_PERIPHERAL_TO_CENTRAL(iqs9151_split_response, ir, source);
ZMK_RELAY_EVENT_HANDLE(iqs9151_split_response, ir, source);

#if IS_ENABLED(CONFIG_INPUT_IQS9151)

static int8_t status_to_i8(int status) {
    return status < INT8_MIN ? INT8_MIN : (status > INT8_MAX ? INT8_MAX : status);
}

static uint8_t split_request_buf[sizeof(struct iqs9151_runtime_config)];
static uint8_t split_request_len;
static uint8_t split_request_seq_active;
static uint8_t split_request_source;

static bool config_is_valid(const struct iqs9151_runtime_config *config) {
    return config->resolution_x <= 4095U &&
           config->resolution_y <= 4095U &&
           config->ati_target_count <= 1000U &&
           config->dynamic_filter_bottom_speed <= 2047U &&
           config->dynamic_filter_top_speed <= 2047U &&
           config->dynamic_filter_bottom_beta <= 255U;
}

static void raise_split_response_chunks(uint8_t seq, int status,
                                        const struct iqs9151_runtime_config *config) {
    const uint8_t *bytes = (const uint8_t *)config;
    const uint8_t total_len = (status == 0 && config != NULL) ? sizeof(*config) : 0;
    const uint8_t chunk_count =
        total_len == 0 ? 1 : DIV_ROUND_UP(total_len, IQS9151_SPLIT_DATA_SIZE);

    for (uint8_t i = 0; i < chunk_count; i++) {
        const uint8_t offset = i * IQS9151_SPLIT_DATA_SIZE;
        const uint8_t remaining = total_len > offset ? total_len - offset : 0;
        const uint8_t data_len = MIN(remaining, IQS9151_SPLIT_DATA_SIZE);
        struct iqs9151_split_response resp = {
            .source = ZMK_RELAY_EVENT_SOURCE_SELF,
            .seq = seq,
            .status = status_to_i8(status),
            .chunk = i,
            .total_len = total_len,
        };

        if (i == chunk_count - 1) {
            resp.chunk |= IQS9151_SPLIT_CHUNK_DONE;
        }
        if (data_len > 0) {
            memcpy(resp.data, bytes + offset, data_len);
        }

        raise_iqs9151_split_response(resp);
        k_msleep(2);
    }
}

static int handle_split_request_event(const zmk_event_t *eh) {
    struct iqs9151_split_request *req = as_iqs9151_split_request(eh);
    if (req == NULL || req->source == ZMK_RELAY_EVENT_SOURCE_SELF) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    int status = 0;
    struct iqs9151_runtime_config config = *iqs9151_runtime_config_get();

    switch (req->type) {
    case IQS9151_SPLIT_REQUEST_GET:
        raise_split_response_chunks(req->seq, 0, &config);
        return ZMK_EV_EVENT_HANDLED;
    case IQS9151_SPLIT_REQUEST_RESET:
        iqs9151_runtime_config_reset();
        config = *iqs9151_runtime_config_get();
        raise_split_response_chunks(req->seq, 0, &config);
        return ZMK_EV_EVENT_HANDLED;
    case IQS9151_SPLIT_REQUEST_SET: {
        const uint8_t chunk = req->chunk & ~IQS9151_SPLIT_CHUNK_DONE;
        if (chunk == 0) {
            split_request_source = req->source;
            split_request_seq_active = req->seq;
            split_request_len = req->total_len;
            memset(split_request_buf, 0, sizeof(split_request_buf));
        } else if (req->source != split_request_source || req->seq != split_request_seq_active) {
            return ZMK_EV_EVENT_BUBBLE;
        }

        const uint8_t offset = chunk * IQS9151_SPLIT_DATA_SIZE;
        if (req->total_len != split_request_len ||
            req->total_len != sizeof(struct iqs9151_runtime_config) ||
            offset > sizeof(split_request_buf)) {
            status = -EOVERFLOW;
            break;
        }

        const uint8_t remaining = req->total_len > offset ? req->total_len - offset : 0;
        const uint8_t data_len = MIN(remaining, IQS9151_SPLIT_DATA_SIZE);
        if (data_len > 0) {
            memcpy(split_request_buf + offset, req->data, data_len);
        }

        if ((req->chunk & IQS9151_SPLIT_CHUNK_DONE) == 0) {
            return ZMK_EV_EVENT_HANDLED;
        }

        memcpy(&config, split_request_buf, sizeof(config));
        if (!config_is_valid(&config)) {
            status = -EINVAL;
            break;
        }

        status = iqs9151_runtime_config_set(&config);
        if (status == 0) {
            config = *iqs9151_runtime_config_get();
        }
        raise_split_response_chunks(req->seq, status, status == 0 ? &config : NULL);
        return ZMK_EV_EVENT_HANDLED;
    }
    default:
        status = -ENOTSUP;
        break;
    }

    raise_split_response_chunks(req->seq, status, NULL);
    return ZMK_EV_EVENT_HANDLED;
}

ZMK_LISTENER(iqs9151_split_request_handler, handle_split_request_event);
ZMK_SUBSCRIPTION(iqs9151_split_request_handler, iqs9151_split_request);

#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

K_SEM_DEFINE(iqs9151_split_response_sem, 0, 1);
static int split_response_status;
static uint8_t split_response_buf[sizeof(struct iqs9151_runtime_config)];
static uint8_t split_response_len;
static uint8_t split_response_source;
static uint8_t split_request_seq;

static int handle_split_response_event(const zmk_event_t *eh) {
    struct iqs9151_split_response *resp = as_iqs9151_split_response(eh);
    if (resp == NULL || resp->source == ZMK_RELAY_EVENT_SOURCE_SELF) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (resp->seq != split_request_seq) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const uint8_t chunk = resp->chunk & ~IQS9151_SPLIT_CHUNK_DONE;
    if (chunk == 0) {
        split_response_source = resp->source;
    } else if (resp->source != split_response_source) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    const uint8_t offset = chunk * IQS9151_SPLIT_DATA_SIZE;
    if (resp->total_len > sizeof(split_response_buf) || offset > sizeof(split_response_buf)) {
        split_response_status = -EOVERFLOW;
        k_sem_give(&iqs9151_split_response_sem);
        return ZMK_EV_EVENT_HANDLED;
    }

    const uint8_t remaining = resp->total_len > offset ? resp->total_len - offset : 0;
    const uint8_t data_len = MIN(remaining, IQS9151_SPLIT_DATA_SIZE);
    if (data_len > 0) {
        memcpy(split_response_buf + offset, resp->data, data_len);
    }

    split_response_status = resp->status;
    split_response_len = resp->total_len;

    if ((resp->chunk & IQS9151_SPLIT_CHUNK_DONE) != 0) {
        k_sem_give(&iqs9151_split_response_sem);
    }
    return ZMK_EV_EVENT_HANDLED;
}

static int call_split_iqs9151(uint8_t type, const struct iqs9151_runtime_config *config,
                              struct iqs9151_runtime_config *out) {
    split_request_seq++;
    if (split_request_seq == 0) {
        split_request_seq = 1;
    }

    while (k_sem_take(&iqs9151_split_response_sem, K_NO_WAIT) == 0) {
    }
    memset(split_response_buf, 0, sizeof(split_response_buf));
    split_response_len = 0;
    split_response_status = 0;
    split_response_source = 0;

    if (config != NULL) {
        const uint8_t *bytes = (const uint8_t *)config;
        const uint8_t total_len = sizeof(*config);
        const uint8_t chunk_count = DIV_ROUND_UP(total_len, IQS9151_SPLIT_DATA_SIZE);

        for (uint8_t i = 0; i < chunk_count; i++) {
            const uint8_t offset = i * IQS9151_SPLIT_DATA_SIZE;
            const uint8_t remaining = total_len > offset ? total_len - offset : 0;
            const uint8_t data_len = MIN(remaining, IQS9151_SPLIT_DATA_SIZE);
            struct iqs9151_split_request req = {
                .source = ZMK_RELAY_EVENT_SOURCE_SELF,
                .seq = split_request_seq,
                .type = type,
                .chunk = (uint8_t)(i | (i == chunk_count - 1 ? IQS9151_SPLIT_CHUNK_DONE : 0)),
                .total_len = total_len,
            };

            if (data_len > 0) {
                memcpy(req.data, bytes + offset, data_len);
            }

            int rc = raise_iqs9151_split_request(req);
            if (rc != 0) {
                return rc;
            }
            k_msleep(12);
        }
    } else {
        struct iqs9151_split_request req = {
            .source = ZMK_RELAY_EVENT_SOURCE_SELF,
            .seq = split_request_seq,
            .type = type,
            .chunk = IQS9151_SPLIT_CHUNK_DONE,
            .total_len = 0,
        };

        int rc = raise_iqs9151_split_request(req);
        if (rc != 0) {
            return rc;
        }
    }

    int rc = k_sem_take(&iqs9151_split_response_sem, K_MSEC(IQS9151_SPLIT_TIMEOUT_MS));
    if (rc != 0) {
        return -ETIMEDOUT;
    }

    if (split_response_status != 0) {
        return split_response_status;
    }
    if (out == NULL) {
        return 0;
    }
    if (split_response_len != sizeof(*out)) {
        return -EIO;
    }

    memcpy(out, split_response_buf, sizeof(*out));
    return 0;
}

int iqs9151_split_get_config(struct iqs9151_runtime_config *out) {
    return call_split_iqs9151(IQS9151_SPLIT_REQUEST_GET, NULL, out);
}

int iqs9151_split_set_config(const struct iqs9151_runtime_config *config,
                             struct iqs9151_runtime_config *out) {
    return call_split_iqs9151(IQS9151_SPLIT_REQUEST_SET, config, out);
}

int iqs9151_split_reset_config(struct iqs9151_runtime_config *out) {
    return call_split_iqs9151(IQS9151_SPLIT_REQUEST_RESET, NULL, out);
}

ZMK_LISTENER(iqs9151_split_response_handler, handle_split_response_event);
ZMK_SUBSCRIPTION(iqs9151_split_response_handler, iqs9151_split_response);

#endif
