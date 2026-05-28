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
    stc8h_u8 release_ticks;
} app_button_t;

#define APP_BUTTON_RELEASE_TICKS 5u

static void app_button_init(app_button_t *button)
{
    button->press_ticks = 0u;
    button->click_ticks = 0u;
    button->was_active = 0u;
    button->click_pending = 0u;
    button->long_sent = 0u;
    button->release_ticks = 0u;
}

static app_button_event_t app_button_update(app_button_t *button,
                                            stc8h_u8 active,
                                            stc8h_u16 long_ticks,
                                            stc8h_u16 double_ticks)
{
    app_button_event_t event;

    event = APP_BUTTON_EVENT_NONE;
    if (active != 0u) {
        button->release_ticks = 0u;
        if (button->press_ticks < long_ticks) {
            ++button->press_ticks;
            if ((button->press_ticks >= long_ticks) && (button->long_sent == 0u)) {
                button->long_sent = 1u;
                button->click_pending = 0u;
                event = APP_BUTTON_EVENT_LONG;
            }
        }
        button->was_active = 1u;
    } else {
        if (button->was_active != 0u) {
            if (button->release_ticks < APP_BUTTON_RELEASE_TICKS) {
                ++button->release_ticks;
            }
            if (button->release_ticks < APP_BUTTON_RELEASE_TICKS) {
                return event;
            }
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
            button->release_ticks = 0u;
            button->was_active = 0u;
        } else if ((button->click_pending != 0u) && (double_ticks != 0u)) {
            ++button->click_ticks;
            if (button->click_ticks >= double_ticks) {
                button->click_pending = 0u;
                event = APP_BUTTON_EVENT_SHORT;
            }
        }
    }

    return event;
}

#endif
