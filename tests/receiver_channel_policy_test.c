#include <assert.h>

#include "app_config.h"

#ifndef APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS
#error "receiver must keep clear-binding input separate from channel-maintenance buttons."
#endif

int main(void)
{
    app_config_t config;

    config.bound_tx_id = 0x4A21u;
    config.rf_channel = 32u;

#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS
    assert(app_config_runtime_channel(&config) == config.rf_channel);
#else
    assert(app_config_runtime_channel(&config) == APP_CONFIG_DEFAULT_CHANNEL);
#endif
    assert(APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS == 1);
    return 0;
}
