#ifndef CONTROLLER_APP_CONFIG_H
#define CONTROLLER_APP_CONFIG_H

#include "stc8h_config.h"

#ifndef APP_TX_ID
#define APP_TX_ID 0x4A21u
#endif

#ifndef APP_DEFAULT_RF_CHANNEL
#define APP_DEFAULT_RF_CHANNEL 76u
#endif

#define APP_CONFIG_FLAG_STEERING_REVERSE 0x01u
#define APP_CONFIG_FLAG_DIRECTION_REVERSE 0x02u
#define APP_CONFIG_STEERING_TRIM_MIN -20
#define APP_CONFIG_STEERING_TRIM_MAX 20
#define APP_CONFIG_STEERING_TRIM_STORAGE_BIAS 20
#define APP_CONFIG_STEERING_REDUCE_MAX 60u
#define APP_CONFIG_STEERING_DEADBAND_MIN 3u
#define APP_CONFIG_STEERING_DEADBAND_MAX 45u
#define APP_CONFIG_DEFAULT_STEERING_TRIM 0
#define APP_CONFIG_DEFAULT_STEERING_REDUCE 10u
#define APP_CONFIG_DEFAULT_STEERING_DEADBAND 10u

typedef struct {
    stc8h_u16 tx_id;
    stc8h_u8 last_channel;
    stc8h_u8 flags;
    stc8h_s8 steering_trim;
    stc8h_u8 steering_deadband;
    stc8h_u8 steering_reduce;
} app_config_t;

stc8h_status_t app_config_load(STC8H_XDATA app_config_t *config);
stc8h_status_t app_config_save(const STC8H_XDATA app_config_t *config);

#endif
