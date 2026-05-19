#ifndef CONTROLLER_APP_BUTTON_H
#define CONTROLLER_APP_BUTTON_H

#include "stc8h_config.h"

typedef enum {
    APP_BUTTON_EVENT_NONE = 0,
    APP_BUTTON_EVENT_SHORT = 1,
    APP_BUTTON_EVENT_DOUBLE = 2,
    APP_BUTTON_EVENT_LONG = 3
} app_button_event_t;

typedef struct {
    stc8h_u16 press_ticks;
    stc8h_u16 click_ticks;
    stc8h_u8 was_active;
    stc8h_u8 click_pending;
    stc8h_u8 long_sent;
} app_button_t;

static void app_button_init(app_button_t *button)
{
    button->press_ticks = 0u;
    button->click_ticks = 0u;
    button->was_active = 0u;
    button->click_pending = 0u;
    button->long_sent = 0u;
}

static app_button_event_t app_button_update(app_button_t *button,
                                            stc8h_u8 active,
                                            stc8h_u16 long_ticks,
                                            stc8h_u16 double_ticks)
{
    app_button_event_t event;

    event = APP_BUTTON_EVENT_NONE;
    if (active != 0u) {
        if (button->press_ticks < long_ticks) {
            ++button->press_ticks;
            if ((button->press_ticks >= long_ticks) && (button->long_sent == 0u)) {
                button->long_sent = 1u;
                button->click_pending = 0u;
                event = APP_BUTTON_EVENT_LONG;
            }
        }
    } else {
        if (button->was_active != 0u) {
            if (button->long_sent == 0u) {
                if (double_ticks == 0u) {
                    event = APP_BUTTON_EVENT_SHORT;
                } else if (button->click_pending != 0u) {
                    button->click_pending = 0u;
                    event = APP_BUTTON_EVENT_DOUBLE;
                } else {
                    button->click_pending = 1u;
                    button->click_ticks = 0u;
                }
            }
            button->press_ticks = 0u;
            button->long_sent = 0u;
        } else if ((button->click_pending != 0u) && (double_ticks != 0u)) {
            ++button->click_ticks;
            if (button->click_ticks >= double_ticks) {
                button->click_pending = 0u;
                event = APP_BUTTON_EVENT_SHORT;
            }
        }
    }

    button->was_active = (active != 0u) ? 1u : 0u;
    return event;
}

#endif
