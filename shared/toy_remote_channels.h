#ifndef TOY_REMOTE_CHANNELS_H
#define TOY_REMOTE_CHANNELS_H

#include "stc8h_config.h"

#define TOY_REMOTE_DEFAULT_RF_CHANNEL 76u
#define TOY_REMOTE_CHANNEL_POOL_COUNT 16u

static stc8h_u8 toy_remote_channel_pool_value(stc8h_u8 index)
{
    if (index >= TOY_REMOTE_CHANNEL_POOL_COUNT) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return (stc8h_u8)(TOY_REMOTE_DEFAULT_RF_CHANNEL - (stc8h_u8)(index << 2));
}

static stc8h_u8 toy_remote_channel_pool_next(stc8h_u8 channel)
{
    if ((channel <= 16u) ||
        (channel > TOY_REMOTE_DEFAULT_RF_CHANNEL) ||
        ((channel & 0x03u) != 0u)) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return (stc8h_u8)(channel - 4u);
}

static stc8h_u8 toy_remote_channel_pool_prev(stc8h_u8 channel)
{
    if ((channel < 16u) ||
        (channel > TOY_REMOTE_DEFAULT_RF_CHANNEL) ||
        ((channel & 0x03u) != 0u)) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    if (channel == TOY_REMOTE_DEFAULT_RF_CHANNEL) {
        return 16u;
    }
    return (stc8h_u8)(channel + 4u);
}

#endif
