#include "app_indicator.h"

#define APP_INDICATOR_PATTERN_BOOT 0u
#define APP_INDICATOR_PATTERN_WAITING_UNBOUND 1u
#define APP_INDICATOR_PATTERN_WAITING_BOUND 2u
#define APP_INDICATOR_PATTERN_CONNECTING 3u
#define APP_INDICATOR_PATTERN_CONNECTED 4u
#define APP_INDICATOR_PATTERN_RADIO_ERROR 5u
#define APP_INDICATOR_PATTERN_BINDING_CLEARED 6u

#define APP_INDICATOR_BOOT_HALF_MS 100u
#define APP_INDICATOR_UNBOUND_SHORT_MS 100u
#define APP_INDICATOR_UNBOUND_PAUSE_MS 900u
#define APP_INDICATOR_BOUND_HALF_MS 500u
#define APP_INDICATOR_CONNECTING_HALF_MS 100u
#define APP_INDICATOR_ERROR_SHORT_MS 100u
#define APP_INDICATOR_ERROR_PAUSE_MS 700u
#define APP_INDICATOR_BOOT_EDGE_COUNT 6u
#define APP_INDICATOR_BINDING_CLEARED_EDGE_COUNT 12u

static stc8h_u16 app_indicator_error_interval(stc8h_u8 step)
{
    return (step == 3u) ? APP_INDICATOR_ERROR_PAUSE_MS : APP_INDICATOR_ERROR_SHORT_MS;
}

static stc8h_u8 app_indicator_error_led(stc8h_u8 step)
{
    return ((step == 0u) || (step == 2u)) ? APP_INDICATOR_LED_ON : APP_INDICATOR_LED_OFF;
}

static void app_indicator_start_timer(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now, stc8h_u16 interval)
{
    indicator->last_tick = now;
    indicator->interval = interval;
}

static stc8h_u8 app_indicator_timer_expired(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now)
{
    if ((stc8h_u16)(now - indicator->last_tick) < indicator->interval) {
        return STC8H_FALSE;
    }

    indicator->last_tick = now;
    return STC8H_TRUE;
}

static void app_indicator_enter_pattern(app_indicator_t STC8H_XDATA *indicator, app_indicator_state_t state, stc8h_u16 now)
{
    indicator->requested_state = state;
    indicator->error_step = 0u;

    if (state == APP_INDICATOR_STATE_CONNECTED) {
        indicator->pattern = APP_INDICATOR_PATTERN_CONNECTED;
        indicator->led_on = APP_INDICATOR_LED_ON;
    } else if (state == APP_INDICATOR_STATE_CONNECTING) {
        indicator->pattern = APP_INDICATOR_PATTERN_CONNECTING;
        indicator->led_on = APP_INDICATOR_LED_ON;
        app_indicator_start_timer(indicator, now, APP_INDICATOR_CONNECTING_HALF_MS);
    } else if (state == APP_INDICATOR_STATE_WAITING_BOUND) {
        indicator->pattern = APP_INDICATOR_PATTERN_WAITING_BOUND;
        indicator->led_on = APP_INDICATOR_LED_ON;
        app_indicator_start_timer(indicator, now, APP_INDICATOR_BOUND_HALF_MS);
    } else if (state == APP_INDICATOR_STATE_BINDING_CLEARED) {
        indicator->pattern = APP_INDICATOR_PATTERN_BINDING_CLEARED;
        indicator->led_on = APP_INDICATOR_LED_ON;
        indicator->boot_edges_remaining = APP_INDICATOR_BINDING_CLEARED_EDGE_COUNT;
        app_indicator_start_timer(indicator, now, APP_INDICATOR_CONNECTING_HALF_MS);
    } else if (state == APP_INDICATOR_STATE_RADIO_ERROR) {
        indicator->pattern = APP_INDICATOR_PATTERN_RADIO_ERROR;
        indicator->led_on = APP_INDICATOR_LED_ON;
        app_indicator_start_timer(indicator, now, APP_INDICATOR_ERROR_SHORT_MS);
    } else {
        indicator->pattern = APP_INDICATOR_PATTERN_WAITING_UNBOUND;
        indicator->led_on = APP_INDICATOR_LED_ON;
        app_indicator_start_timer(indicator, now, APP_INDICATOR_UNBOUND_SHORT_MS);
    }
}

void app_indicator_init(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now)
{
    indicator->requested_state = APP_INDICATOR_STATE_WAITING_UNBOUND;
    indicator->pattern = APP_INDICATOR_PATTERN_BOOT;
    indicator->led_on = APP_INDICATOR_LED_ON;
    indicator->boot_edges_remaining = APP_INDICATOR_BOOT_EDGE_COUNT;
    indicator->error_step = 0u;
    app_indicator_start_timer(indicator, now, APP_INDICATOR_BOOT_HALF_MS);
}

void app_indicator_set_state(app_indicator_t STC8H_XDATA *indicator, app_indicator_state_t state, stc8h_u16 now)
{
    if ((indicator->requested_state == state) &&
        ((state != APP_INDICATOR_STATE_CONNECTED) ||
         (indicator->pattern != APP_INDICATOR_PATTERN_BOOT))) {
        return;
    }

    indicator->requested_state = state;
    if ((indicator->pattern != APP_INDICATOR_PATTERN_BOOT) ||
        (state == APP_INDICATOR_STATE_CONNECTED)) {
        app_indicator_enter_pattern(indicator, state, now);
    }
}

stc8h_u8 app_indicator_update(app_indicator_t STC8H_XDATA *indicator, stc8h_u16 now)
{
    if (indicator->pattern == APP_INDICATOR_PATTERN_BOOT) {
        if (app_indicator_timer_expired(indicator, now) != 0u) {
            indicator->led_on = (indicator->led_on == APP_INDICATOR_LED_ON) ? APP_INDICATOR_LED_OFF : APP_INDICATOR_LED_ON;
            --indicator->boot_edges_remaining;
            if (indicator->boot_edges_remaining == 0u) {
                app_indicator_enter_pattern(indicator, indicator->requested_state, now);
            }
        }
    } else if (indicator->pattern == APP_INDICATOR_PATTERN_WAITING_UNBOUND) {
        if (app_indicator_timer_expired(indicator, now) != 0u) {
            if (indicator->led_on == APP_INDICATOR_LED_ON) {
                indicator->led_on = APP_INDICATOR_LED_OFF;
                indicator->interval = APP_INDICATOR_UNBOUND_PAUSE_MS;
            } else {
                indicator->led_on = APP_INDICATOR_LED_ON;
                indicator->interval = APP_INDICATOR_UNBOUND_SHORT_MS;
            }
        }
    } else if ((indicator->pattern == APP_INDICATOR_PATTERN_WAITING_BOUND) ||
               (indicator->pattern == APP_INDICATOR_PATTERN_CONNECTING)) {
        if (app_indicator_timer_expired(indicator, now) != 0u) {
            indicator->led_on = (indicator->led_on == APP_INDICATOR_LED_ON) ? APP_INDICATOR_LED_OFF : APP_INDICATOR_LED_ON;
        }
    } else if (indicator->pattern == APP_INDICATOR_PATTERN_CONNECTED) {
        indicator->led_on = APP_INDICATOR_LED_ON;
    } else if (indicator->pattern == APP_INDICATOR_PATTERN_BINDING_CLEARED) {
        if (app_indicator_timer_expired(indicator, now) != 0u) {
            indicator->led_on = (indicator->led_on == APP_INDICATOR_LED_ON) ? APP_INDICATOR_LED_OFF : APP_INDICATOR_LED_ON;
            --indicator->boot_edges_remaining;
            if (indicator->boot_edges_remaining == 0u) {
                app_indicator_enter_pattern(indicator, APP_INDICATOR_STATE_WAITING_UNBOUND, now);
            }
        }
    } else {
        if (app_indicator_timer_expired(indicator, now) != 0u) {
            ++indicator->error_step;
            if (indicator->error_step > 3u) {
                indicator->error_step = 0u;
            }
            indicator->led_on = app_indicator_error_led(indicator->error_step);
            indicator->interval = app_indicator_error_interval(indicator->error_step);
        }
    }

    return indicator->led_on;
}
