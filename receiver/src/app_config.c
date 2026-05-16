#include "app_config.h"
#include "stc8h_eeprom.h"

#define APP_CONFIG_ADDR 0u
#define APP_CONFIG_LEN 9u
#define APP_CONFIG_MAGIC0 0x52u
#define APP_CONFIG_MAGIC1 0x43u
#define APP_CONFIG_VERSION 1u
#define APP_CONFIG_DEFAULT_CHANNEL 40u

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
    config->bound_tx_id = 0u;
    config->rf_channel = APP_CONFIG_DEFAULT_CHANNEL;
    config->servo_reverse = 0u;
}

static stc8h_status_t app_config_validate(app_config_t *config)
{
    if (config == 0) {
        return STC8H_ERROR;
    }
    if ((config->rf_channel > 125u) || (config->servo_reverse > 1u)) {
        app_config_set_defaults(config);
        return STC8H_ERROR;
    }
    return STC8H_OK;
}

stc8h_status_t app_config_load(app_config_t *config)
{
    if (config == 0) {
        return STC8H_ERROR;
    }

    if (stc8h_eeprom_read(APP_CONFIG_ADDR, app_config_buf, APP_CONFIG_LEN) != STC8H_OK) {
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

    config->bound_tx_id = (stc8h_u16)((stc8h_u16)app_config_buf[4] | ((stc8h_u16)app_config_buf[5] << 8));
    config->rf_channel = app_config_buf[6];
    config->servo_reverse = app_config_buf[7];
    return app_config_validate(config);
}

stc8h_status_t app_config_save(const app_config_t *config)
{
    if ((config == 0) || (config->rf_channel > 125u) || (config->servo_reverse > 1u)) {
        return STC8H_ERROR;
    }

    app_config_buf[0] = APP_CONFIG_MAGIC0;
    app_config_buf[1] = APP_CONFIG_MAGIC1;
    app_config_buf[2] = APP_CONFIG_VERSION;
    app_config_buf[3] = APP_CONFIG_LEN;
    app_config_buf[4] = (stc8h_u8)config->bound_tx_id;
    app_config_buf[5] = (stc8h_u8)(config->bound_tx_id >> 8);
    app_config_buf[6] = config->rf_channel;
    app_config_buf[7] = config->servo_reverse;
    app_config_buf[8] = app_config_checksum();

    if (stc8h_eeprom_erase_sector(APP_CONFIG_ADDR) != STC8H_OK) {
        return STC8H_ERROR;
    }
    return stc8h_eeprom_write(APP_CONFIG_ADDR, app_config_buf, APP_CONFIG_LEN);
}
