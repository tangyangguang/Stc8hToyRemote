#ifndef TOY_REMOTE_CHANNELS_H
#define TOY_REMOTE_CHANNELS_H

#include "stc8h_config.h"

#define TOY_REMOTE_DEFAULT_RF_CHANNEL 76u
#define TOY_REMOTE_CHANNEL_POOL_MIN 16u
#define TOY_REMOTE_CHANNEL_POOL_COUNT 16u
#define toy_remote_channel_pool_contains(channel) \
    ((((channel) >= TOY_REMOTE_CHANNEL_POOL_MIN) && \
      ((channel) <= TOY_REMOTE_DEFAULT_RF_CHANNEL) && \
      (((channel) & 0x03u) == 0u)) ? 1u : 0u)

#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE 1
#endif
#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT 1
#endif
#ifndef TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV 1
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_VALUE
static stc8h_u8 toy_remote_channel_pool_value(stc8h_u8 index)
{
    if (index >= TOY_REMOTE_CHANNEL_POOL_COUNT) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return (stc8h_u8)(TOY_REMOTE_DEFAULT_RF_CHANNEL - (stc8h_u8)(index << 2));
}
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT
static stc8h_u8 toy_remote_channel_pool_next(stc8h_u8 channel)
{
    if ((channel <= TOY_REMOTE_CHANNEL_POOL_MIN) ||
        (channel > TOY_REMOTE_DEFAULT_RF_CHANNEL) ||
        ((channel & 0x03u) != 0u)) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    return (stc8h_u8)(channel - 4u);
}
#endif

#if TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV
static stc8h_u8 toy_remote_channel_pool_prev(stc8h_u8 channel)
{
    if ((channel < TOY_REMOTE_CHANNEL_POOL_MIN) ||
        (channel > TOY_REMOTE_DEFAULT_RF_CHANNEL) ||
        ((channel & 0x03u) != 0u)) {
        return TOY_REMOTE_DEFAULT_RF_CHANNEL;
    }
    if (channel == TOY_REMOTE_DEFAULT_RF_CHANNEL) {
        return TOY_REMOTE_CHANNEL_POOL_MIN;
    }
    return (stc8h_u8)(channel + 4u);
}
#endif

#endif
