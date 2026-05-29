#include "app_config.h"
#include "stc8h_eeprom.h"

#include <assert.h>
#include <string.h>

#define TEST_CONFIG_LEN 9u
#define TEST_CONFIG_MAGIC0 0x52u
#define TEST_CONFIG_MAGIC1 0x43u
#define TEST_CONFIG_VERSION 2u

static stc8h_u8 eeprom_image[TEST_CONFIG_LEN];
static stc8h_u8 saved_image[TEST_CONFIG_LEN];
static stc8h_status_t read_result;
static stc8h_u8 save_calls;

static stc8h_u8 test_checksum(const stc8h_u8 *data)
{
    stc8h_u8 i;
    stc8h_u8 sum;

    sum = 0u;
    for (i = 0u; i < (TEST_CONFIG_LEN - 1u); ++i) {
        sum = (stc8h_u8)(sum + data[i]);
    }
    return (stc8h_u8)(0u - sum);
}

static void seed_config(stc8h_u16 bound_tx_id, stc8h_u8 rf_channel)
{
    memset(eeprom_image, 0, sizeof(eeprom_image));
    eeprom_image[0] = TEST_CONFIG_MAGIC0;
    eeprom_image[1] = TEST_CONFIG_MAGIC1;
    eeprom_image[2] = TEST_CONFIG_VERSION;
    eeprom_image[3] = TEST_CONFIG_LEN;
    eeprom_image[4] = (stc8h_u8)bound_tx_id;
    eeprom_image[5] = (stc8h_u8)(bound_tx_id >> 8);
    eeprom_image[6] = rf_channel;
    eeprom_image[7] = 0u;
    eeprom_image[8] = test_checksum(eeprom_image);
}

stc8h_status_t stc8h_eeprom_read_fixed(STC8H_DATA stc8h_u8 *data)
{
    memcpy(data, eeprom_image, sizeof(eeprom_image));
    return read_result;
}

stc8h_status_t stc8h_eeprom_save_fixed(const STC8H_DATA stc8h_u8 *data)
{
    memcpy(saved_image, data, sizeof(saved_image));
    ++save_calls;
    return STC8H_OK;
}

static void reset_stubs(void)
{
    read_result = STC8H_OK;
    save_calls = 0u;
    memset(saved_image, 0, sizeof(saved_image));
}

static void test_load_accepts_saved_channel_from_pool(void)
{
    app_config_t config;

    reset_stubs();
    seed_config(0x4A21u, 67u);

    assert(app_config_load(&config) == STC8H_OK);
    assert(config.bound_tx_id == 0x4A21u);
    assert(config.rf_channel == 67u);
}

static void test_load_recovers_channel_outside_pool_to_default(void)
{
    app_config_t config;

    reset_stubs();
    seed_config(0x4A21u, 20u);

    assert(app_config_load(&config) == STC8H_ERROR);
    assert(config.bound_tx_id == 0x4A21u);
    assert(config.rf_channel == APP_CONFIG_DEFAULT_CHANNEL);
}

static void test_save_writes_current_channel(void)
{
    app_config_t config;

    reset_stubs();
    config.bound_tx_id = 0x1357u;
    config.rf_channel = 36u;

    assert(app_config_save(&config) == STC8H_OK);
    assert(save_calls == 1u);
    assert(saved_image[4] == 0x57u);
    assert(saved_image[5] == 0x13u);
    assert(saved_image[6] == 36u);
    assert(saved_image[8] == test_checksum(saved_image));
}

int main(void)
{
    test_load_accepts_saved_channel_from_pool();
    test_load_recovers_channel_outside_pool_to_default();
    test_save_writes_current_channel();
    return 0;
}
