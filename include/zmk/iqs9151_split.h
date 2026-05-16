#pragma once

#include <zephyr/sys/util.h>
#include <zmk/iqs9151_runtime.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_RELAY_EVENT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
int iqs9151_split_get_config(struct iqs9151_runtime_config *out);
int iqs9151_split_set_config(const struct iqs9151_runtime_config *config,
                             struct iqs9151_runtime_config *out);
int iqs9151_split_reset_config(struct iqs9151_runtime_config *out);
#endif
