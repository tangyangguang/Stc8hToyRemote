#ifndef TOY_REMOTE_CHANNELS_H
#define TOY_REMOTE_CHANNELS_H

#include "stc8h_config.h"

#define TOY_REMOTE_DEFAULT_RF_CHANNEL 76u
#define TOY_REMOTE_CHANNEL_POOL_COUNT 32u
#define TOY_REMOTE_CHANNEL_POOL_INVALID_INDEX 0xFFu

#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_CONTAINS
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_CONTAINS 1
#endif
#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE 1
#endif
#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT 1
#endif
#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV 1
#endif

static STC8H_CODE stc8h_u8 toy_remote_channel_pool_values[TOY_REMOTE_CHANNEL_POOL_COUNT] = {
    76u, 36u, 40u, 44u, 48u, 52u, 56u, 60u,
    64u, 68u, 72u, 80u, 83u, 32u, 34u, 38u,
    42u, 46u, 50u, 54u, 58u, 62u, 66u, 70u,
    74u, 78u, 82u, 35u, 43u, 51u, 59u, 67u
};

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_CONTAINS || \
    TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT || \
    TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV
static stc8h_u8 toy_remote_channel_pool_index(stc8h_u8 channel)
{
    stc8h_u8 index;

    for (index = 0u; index < TOY_REMOTE_CHANNEL_POOL_COUNT; ++index) {
        if (toy_remote_channel_pool_values[index] == channel) {
            return index;
        }
    }
    return TOY_REMOTE_CHANNEL_POOL_INVALID_INDEX;
}
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_CONTAINS
#define toy_remote_channel_pool_contains(channel) \
    ((toy_remote_channel_pool_index((channel)) == TOY_REMOTE_CHANNEL_POOL_INVALID_INDEX) ? 0u : 1u)
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE
static stc8h_u8 toy_remote_channel_pool_value(stc8h_u8 index)
{
    if (index >= TOY_REMOTE_CHANNEL_POOL_COUNT) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return toy_remote_channel_pool_values[index];
}
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT
static stc8h_u8 toy_remote_channel_pool_next(stc8h_u8 channel)
{
    stc8h_u8 index;

    index = toy_remote_channel_pool_index(channel);
    if (index == TOY_REMOTE_CHANNEL_POOL_INVALID_INDEX) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    ++index;
    if (index >= TOY_REMOTE_CHANNEL_POOL_COUNT) {
        index = 0u;
    }
    return toy_remote_channel_pool_values[index];
}
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV
static stc8h_u8 toy_remote_channel_pool_prev(stc8h_u8 channel)
{
    stc8h_u8 index;

    index = toy_remote_channel_pool_index(channel);
    if (index == TOY_REMOTE_CHANNEL_POOL_INVALID_INDEX) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    if (index == 0u) {
        index = TOY_REMOTE_CHANNEL_POOL_COUNT;
    }
    --index;
    return toy_remote_channel_pool_values[index];
}
#endif

#endif
