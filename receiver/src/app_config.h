#ifndef RECEIVER_APP_CONFIG_H
#define RECEIVER_APP_CONFIG_H

#include "stc8h_config.h"

#ifndef APP_CONFIG_DEFAULT_CHANNEL
#define APP_CONFIG_DEFAULT_CHANNEL 76u
#endif

#ifndef APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
#define APP_RECEIVER_ENABLE_CHANNEL_BUTTONS 1
#endif

#ifndef APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS
#define APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS 1
#endif

typedef struct {
    stc8h_u16 bound_tx_id;
    stc8h_u8 rf_channel;
} app_config_t;

#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
#define app_config_runtime_channel(config) ((config)->rf_channel)
#else
#define app_config_runtime_channel(config) ((void)(config), APP_CONFIG_DEFAULT_CHANNEL)
#endif

stc8h_status_t app_config_load(STC8H_XDATA app_config_t *config);
stc8h_status_t app_config_save(const STC8H_XDATA app_config_t *config);

#endif
