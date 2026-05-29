#include <assert.h>

#include "app_config.h"

#ifndef APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS
#error "receiver must keep clear-binding input separate from channel-maintenance buttons."
#endif
#if APP_RECEIVER_ENABLE_CHANNEL_BUTTONS != 1
#error "receiver default build must enable P30/P31 runtime channel buttons."
#endif

int main(void)
{
    app_config_t config;

    config.bound_tx_id = 0x4A21u;
    config.rf_channel = 32u;

    assert(app_config_runtime_channel(&config) == config.rf_channel);
    assert(APP_RECEIVER_ENABLE_CLEAR_BINDING_BUTTONS == 1);
    return 0;
}
