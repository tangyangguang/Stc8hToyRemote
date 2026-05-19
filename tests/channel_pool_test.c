#include "toy_remote_channels.h"

#include <assert.h>

static void test_default_and_pool_order(void)
{
    assert(TOY_REMOTE_DEFAULT_RF_CHANNEL == 76u);
    assert(toy_remote_channel_pool_value(0u) == 76u);
    assert(toy_remote_channel_pool_value(1u) == 72u);
    assert(toy_remote_channel_pool_value(15u) == 16u);
    assert(toy_remote_channel_pool_value(16u) == 76u);
}

static void test_pool_wraps_from_known_channels(void)
{
    assert(toy_remote_channel_pool_next(76u) == 72u);
    assert(toy_remote_channel_pool_prev(76u) == 16u);
    assert(toy_remote_channel_pool_next(40u) == 36u);
    assert(toy_remote_channel_pool_prev(40u) == 44u);
}

static void test_pool_falls_back_to_default_from_unknown_channel(void)
{
    assert(toy_remote_channel_pool_next(41u) == 76u);
    assert(toy_remote_channel_pool_prev(41u) == 76u);
}

int main(void)
{
    test_default_and_pool_order();
    test_pool_wraps_from_known_channels();
    test_pool_falls_back_to_default_from_unknown_channel();
    return 0;
}
