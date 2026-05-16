#ifndef CONTROLLER_APP_CONFIG_H
#define CONTROLLER_APP_CONFIG_H

#include "stc8h_config.h"

#ifndef APP_TX_ID
#define APP_TX_ID 0x4A21u
#endif

#ifndef APP_DEFAULT_RF_CHANNEL
#define APP_DEFAULT_RF_CHANNEL 40u
#endif

typedef struct {
    stc8h_u16 tx_id;
    stc8h_u8 last_channel;
} app_config_t;

stc8h_status_t app_config_load(app_config_t *config);
stc8h_status_t app_config_save(const app_config_t *config);

#endif
