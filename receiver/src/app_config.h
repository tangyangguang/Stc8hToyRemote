#ifndef RECEIVER_APP_CONFIG_H
#define RECEIVER_APP_CONFIG_H

#include "stc8h_config.h"

typedef struct {
    stc8h_u16 bound_tx_id;
    stc8h_u8 rf_channel;
    stc8h_u8 servo_reverse;
} app_config_t;

stc8h_status_t app_config_load(app_config_t *config);
stc8h_status_t app_config_save(const app_config_t *config);

#endif
