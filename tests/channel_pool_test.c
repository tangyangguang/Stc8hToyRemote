#include "toy_remote_channels.h"

#include <assert.h>

static void test_default_and_pool_order(void)
{
    assert(TOY_REMOTE_DEFAULT_RF_CHANNEL == 76u);
    assert(TOY_REMOTE_CHANNEL_POOL_COUNT == 32u);
    assert(toy_remote_channel_pool_value(0u) == 76u);
    assert(toy_remote_channel_pool_value(1u) == 36u);
    assert(toy_remote_channel_pool_value(31u) == 67u);
    assert(toy_remote_channel_pool_value(32u) == 76u);
    assert(toy_remote_channel_pool_contains(76u) == 1u);
    assert(toy_remote_channel_pool_contains(36u) == 1u);
    assert(toy_remote_channel_pool_contains(83u) == 1u);
    assert(toy_remote_channel_pool_contains(80u) == 1u);
    assert(toy_remote_channel_pool_contains(20u) == 0u);
}

static void test_pool_wraps_from_known_channels(void)
{
    assert(toy_remote_channel_pool_next(76u) == 36u);
    assert(toy_remote_channel_pool_prev(76u) == 67u);
    assert(toy_remote_channel_pool_next(40u) == 44u);
    assert(toy_remote_channel_pool_prev(40u) == 36u);
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
