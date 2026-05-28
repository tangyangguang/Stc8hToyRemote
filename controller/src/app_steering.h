#ifndef CONTROLLER_APP_STEERING_H
#define CONTROLLER_APP_STEERING_H

#include "app_config.h"
#include "toy_remote_protocol.h"

static stc8h_u8 app_steering_apply_config(stc8h_u8 raw_angle,
                                          stc8h_u8 flags,
                                          stc8h_s8 steering_trim,
                                          stc8h_u8 steering_deadband,
                                          stc8h_u8 steering_reduce)
{
    stc8h_u8 min_angle;
    stc8h_u8 max_angle;
    stc8h_u8 center_angle;
    stc8h_u8 lower_center;
    stc8h_u8 upper_center;
    stc8h_u8 denominator;
    stc8h_u16 angle;

    if ((flags & APP_CONFIG_FLAG_STEERING_REVERSE) != 0u) {
        raw_angle = (stc8h_u8)(TOY_REMOTE_STEERING_MAX - raw_angle);
    }

    min_angle = steering_reduce;
    max_angle = (stc8h_u8)(TOY_REMOTE_STEERING_MAX - steering_reduce);
    center_angle = (stc8h_u8)((stc8h_s16)TOY_REMOTE_STEERING_CENTER + steering_trim);
    lower_center = (stc8h_u8)(TOY_REMOTE_STEERING_CENTER - steering_deadband);
    upper_center = (stc8h_u8)(TOY_REMOTE_STEERING_CENTER + steering_deadband);

    if ((raw_angle >= lower_center) && (raw_angle <= upper_center)) {
        return center_angle;
    }

    if (raw_angle < lower_center) {
        angle = (stc8h_u16)(min_angle +
            (((stc8h_u16)raw_angle * (stc8h_u16)(center_angle - min_angle)) /
             lower_center));
    } else {
        denominator = (stc8h_u8)(TOY_REMOTE_STEERING_MAX - upper_center);
        angle = (stc8h_u16)(raw_angle - upper_center);
        angle = (stc8h_u16)(center_angle +
            ((angle * (stc8h_u16)(max_angle - center_angle)) / denominator));
    }

    return (stc8h_u8)angle;
}

#endif
