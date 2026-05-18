#ifndef CONTROLLER_APP_EC11_SPEED_H
#define CONTROLLER_APP_EC11_SPEED_H

#include "stc8h_config.h"

#define APP_EC11_SPEED_MEDIUM_HALF_MS 30u

static stc8h_s16 app_ec11_speed_scale_delta(stc8h_s16 delta, stc8h_u16 interval_half_ms, stc8h_u8 enabled)
{
    stc8h_s16 step;

    if ((enabled == 0u) || (delta == 0)) {
        return delta;
    }

    step = 1;
    if (interval_half_ms <= APP_EC11_SPEED_MEDIUM_HALF_MS) {
        step = 5;
    }

    return (stc8h_s16)(delta * step);
}

#endif
