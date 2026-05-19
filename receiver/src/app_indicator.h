#ifndef RECEIVER_APP_INDICATOR_H
#define RECEIVER_APP_INDICATOR_H

#include "stc8h_config.h"

#define APP_INDICATOR_LED_OFF 0u
#define APP_INDICATOR_LED_ON 1u

typedef enum {
    APP_INDICATOR_STATE_WAITING = 0,
    APP_INDICATOR_STATE_CONNECTING = 1,
    APP_INDICATOR_STATE_CONNECTED = 2,
    APP_INDICATOR_STATE_RADIO_ERROR = 3
} app_indicator_state_t;

typedef struct {
    stc8h_u16 last_tick;
    stc8h_u16 interval;
    app_indicator_state_t requested_state;
    stc8h_u8 pattern;
    stc8h_u8 led_on;
    stc8h_u8 boot_edges_remaining;
    stc8h_u8 error_step;
} app_indicator_t;

void app_indicator_init(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now);
void app_indicator_set_state(app_indicator_t STC8H_XDATA *indicator, app_indicator_state_t state, stc8h_u16 now);
stc8h_u8 app_indicator_update(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now);

#endif
