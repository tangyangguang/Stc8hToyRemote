#ifndef CONTROLLER_APP_EC11_SPEED_H
#define CONTROLLER_APP_EC11_SPEED_H

#include "stc8h_config.h"

#define APP_EC11_SPEED_MEDIUM_HALF_MS 30u

#ifndef APP_EC11_SPEED_ENABLE_SCALE_HELPER
#define APP_EC11_SPEED_ENABLE_SCALE_HELPER 1
#endif

#if APP_EC11_SPEED_ENABLE_SCALE_HELPER
static inline stc8h_s16 app_ec11_speed_scale_delta(stc8h_s16 delta, stc8h_u16 interval_half_ms, stc8h_u8 enabled)
{
    if ((enabled == 0u) || (delta == 0)) {
        return delta;
    }

    if (interval_half_ms <= APP_EC11_SPEED_MEDIUM_HALF_MS) {
        return (delta > 0) ? 5 : -5;
    }

    return delta;
}
#endif

#endif
