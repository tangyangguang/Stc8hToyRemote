#ifndef CONTROLLER_APP_STEERING_H
#define CONTROLLER_APP_STEERING_H

#include "app_config.h"
#include "toy_remote_protocol.h"

static stc8h_u8 app_steering_apply_config(stc8h_u8 raw_angle,
                                          stc8h_u8 flags,
                                          stc8h_u8 steering_middle,
                                          stc8h_u8 steering_reduce)
{
    stc8h_u8 min_angle;
    stc8h_u8 max_angle;
    stc8h_u16 center;
    stc8h_u16 angle;

    if ((flags & APP_CONFIG_FLAG_STEERING_REVERSE) != 0u) {
        raw_angle = (stc8h_u8)(TOY_REMOTE_STEERING_MAX - raw_angle);
    }

    min_angle = steering_reduce;
    max_angle = (stc8h_u8)(TOY_REMOTE_STEERING_MAX - steering_reduce);
    center = (stc8h_u16)steering_middle << 1;
    if (center < min_angle) {
        center = min_angle;
    } else if (center > max_angle) {
        center = max_angle;
    }

    if (raw_angle <= TOY_REMOTE_STEERING_CENTER) {
        angle = (stc8h_u16)(min_angle +
            ((((stc8h_u16)raw_angle * (stc8h_u16)(center - min_angle)) + 45u) /
             TOY_REMOTE_STEERING_CENTER));
    } else {
        angle = (stc8h_u16)(center +
            ((((stc8h_u16)(raw_angle - TOY_REMOTE_STEERING_CENTER) *
               (stc8h_u16)(max_angle - center)) + 45u) /
             TOY_REMOTE_STEERING_CENTER));
    }

    if (angle > max_angle) {
        angle = max_angle;
    }
    return (stc8h_u8)angle;
}

#endif
