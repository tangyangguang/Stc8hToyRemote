#include "app_config.h"
#include "toy_remote_channels.h"

#include <assert.h>

int main(void)
{
    assert(APP_CONFIG_DEFAULT_CHANNEL == TOY_REMOTE_DEFAULT_RF_CHANNEL);
    assert(toy_remote_channel_pool_value(0u) == APP_CONFIG_DEFAULT_CHANNEL);
    assert(toy_remote_channel_pool_next(APP_CONFIG_DEFAULT_CHANNEL) == 72u);
    assert(toy_remote_channel_pool_prev(APP_CONFIG_DEFAULT_CHANNEL) == 16u);
    return 0;
}
