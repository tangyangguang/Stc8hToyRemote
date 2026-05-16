#include "app_config.h"
#include "stc8h_eeprom.h"

#define APP_CONFIG_LEN 11u
#define APP_CONFIG_MAGIC0 0x54u
#define APP_CONFIG_MAGIC1 0x43u
#define APP_CONFIG_VERSION 2u

static STC8H_DATA stc8h_u8 app_config_buf[APP_CONFIG_LEN];

static stc8h_u8 app_config_checksum(void)
{
    stc8h_u8 i;
    stc8h_u8 sum;

    sum = 0u;
    for (i = 0u; i < (APP_CONFIG_LEN - 1u); ++i) {
        sum = (stc8h_u8)(sum + app_config_buf[i]);
    }
    return (stc8h_u8)(0u - sum);
}

static void app_config_set_defaults(app_config_t *config)
{
    config->tx_id = APP_TX_ID;
    config->last_channel = APP_DEFAULT_RF_CHANNEL;
    config->flags = 0u;
    config->steering_reduce = APP_CONFIG_DEFAULT_STEERING_REDUCE;
    config->steering_middle = APP_CONFIG_DEFAULT_STEERING_MIDDLE;
}

stc8h_status_t app_config_load(app_config_t *config)
{
    if (config == 0) {
        return STC8H_ERROR;
    }

    if (stc8h_eeprom_read_fixed(app_config_buf) != STC8H_OK) {
        app_config_set_defaults(config);
        return STC8H_ERROR;
    }
    if ((app_config_buf[0] != APP_CONFIG_MAGIC0) ||
        (app_config_buf[1] != APP_CONFIG_MAGIC1) ||
        (app_config_buf[2] != APP_CONFIG_VERSION) ||
        (app_config_buf[3] != APP_CONFIG_LEN) ||
        (app_config_checksum() != app_config_buf[APP_CONFIG_LEN - 1u])) {
        app_config_set_defaults(config);
        return STC8H_ERROR;
    }

    config->tx_id = (stc8h_u16)((stc8h_u16)app_config_buf[4] | ((stc8h_u16)app_config_buf[5] << 8));
    config->last_channel = app_config_buf[6];
    config->flags = app_config_buf[7];
    config->steering_reduce = app_config_buf[8];
    config->steering_middle = app_config_buf[9];
    if ((config->tx_id == 0u) || (config->last_channel > 125u)) {
        app_config_set_defaults(config);
        return STC8H_ERROR;
    }
    config->flags &= (APP_CONFIG_FLAG_STEERING_REVERSE | APP_CONFIG_FLAG_DIRECTION_REVERSE);
    if (config->steering_reduce > APP_CONFIG_STEERING_REDUCE_MAX) {
        config->steering_reduce = APP_CONFIG_DEFAULT_STEERING_REDUCE;
    }
    if ((config->steering_middle < APP_CONFIG_STEERING_MIDDLE_MIN) ||
        (config->steering_middle > APP_CONFIG_STEERING_MIDDLE_MAX)) {
        config->steering_middle = APP_CONFIG_DEFAULT_STEERING_MIDDLE;
    }
    return STC8H_OK;
}

stc8h_status_t app_config_save(const app_config_t *config)
{
    if (config == 0) {
        return STC8H_ERROR;
    }

    app_config_buf[0] = APP_CONFIG_MAGIC0;
    app_config_buf[1] = APP_CONFIG_MAGIC1;
    app_config_buf[2] = APP_CONFIG_VERSION;
    app_config_buf[3] = APP_CONFIG_LEN;
    app_config_buf[4] = (stc8h_u8)config->tx_id;
    app_config_buf[5] = (stc8h_u8)(config->tx_id >> 8);
    app_config_buf[6] = config->last_channel;
    app_config_buf[7] = config->flags;
    app_config_buf[8] = config->steering_reduce;
    app_config_buf[9] = config->steering_middle;
    app_config_buf[10] = app_config_checksum();

    return stc8h_eeprom_save_fixed(app_config_buf);
}
