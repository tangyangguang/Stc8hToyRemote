#include "app_config.h"
#include "toy_remote_channels.h"

#include <assert.h>

int main(void)
{
    assert(APP_DEFAULT_RF_CHANNEL == TOY_REMOTE_DEFAULT_RF_CHANNEL);
    assert(APP_CONFIG_STEERING_TRIM_MIN == -20);
    assert(APP_CONFIG_STEERING_TRIM_MAX == 20);
    assert(APP_CONFIG_DEFAULT_STEERING_TRIM == 0);
    assert(APP_CONFIG_STEERING_DEADBAND_MIN == 3u);
    assert(APP_CONFIG_STEERING_DEADBAND_MAX == 45u);
    assert(APP_CONFIG_DEFAULT_STEERING_DEADBAND == 10u);
    assert(APP_CONFIG_STEERING_REDUCE_MAX == 60u);
    assert(APP_CONFIG_DEFAULT_STEERING_REDUCE == 20u);
    assert(toy_remote_channel_pool_value(0u) == APP_DEFAULT_RF_CHANNEL);
    assert(toy_remote_channel_pool_next(APP_DEFAULT_RF_CHANNEL) == 72u);
    assert(toy_remote_channel_pool_prev(APP_DEFAULT_RF_CHANNEL) == 16u);
    return 0;
}
