#include <assert.h>

#include "app_config.h"

int main(void)
{
    app_config_t config;

    config.bound_tx_id = 0x4A21u;
    config.rf_channel = 32u;
    config.servo_reverse = 0u;

#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
    assert(app_config_runtime_channel(&config) == config.rf_channel);
#else
    assert(app_config_runtime_channel(&config) == APP_CONFIG_DEFAULT_CHANNEL);
#endif
    return 0;
}
