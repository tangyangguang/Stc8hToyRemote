#ifndef TOY_REMOTE_CHANNELS_H
#define TOY_REMOTE_CHANNELS_H

#include "stc8h_config.h"

#define TOY_REMOTE_DEFAULT_RF_CHANNEL 76u
#define TOY_REMOTE_CHANNEL_POOL_COUNT 16u

static stc8h_u8 toy_remote_channel_pool_value(stc8h_u8 index)
{
    static STC8H_CODE stc8h_u8 pool[TOY_REMOTE_CHANNEL_POOL_COUNT] = {
        76u, 72u, 68u, 64u, 60u, 56u, 52u, 48u,
        44u, 40u, 36u, 32u, 28u, 24u, 20u, 16u
    };

    return pool[(index < TOY_REMOTE_CHANNEL_POOL_COUNT) ? index : 0u];
}

static stc8h_u8 toy_remote_channel_pool_index(stc8h_u8 channel)
{
    stc8h_u8 i;

    for (i = 0u; i < TOY_REMOTE_CHANNEL_POOL_COUNT; ++i) {
        if (toy_remote_channel_pool_value(i) == channel) {
            return i;
        }
    }
    return 0xFFu;
}

static stc8h_u8 toy_remote_channel_pool_next(stc8h_u8 channel)
{
    stc8h_u8 index;

    index = toy_remote_channel_pool_index(channel);
    if (index == 0xFFu) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    ++index;
    if (index >= TOY_REMOTE_CHANNEL_POOL_COUNT) {
        index = 0u;
    }
    return toy_remote_channel_pool_value(index);
}

static stc8h_u8 toy_remote_channel_pool_prev(stc8h_u8 channel)
{
    stc8h_u8 index;

    index = toy_remote_channel_pool_index(channel);
    if (index == 0xFFu) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    if (index == 0u) {
        index = TOY_REMOTE_CHANNEL_POOL_COUNT;
    }
    return toy_remote_channel_pool_value((stc8h_u8)(index - 1u));
}

#endif
