#include "app_button.h"

#include <assert.h>

static app_button_event_t update_many(app_button_t *button,
                                      stc8h_u8 active,
                                      stc8h_u16 ticks,
                                      stc8h_u16 long_ticks,
                                      stc8h_u16 double_ticks)
{
    app_button_event_t event;

    event = APP_BUTTON_EVENT_NONE;
    while (ticks != 0u) {
        event = app_button_update(button, active, long_ticks, double_ticks);
        --ticks;
    }
    return event;
}

static app_button_event_t release_many(app_button_t *button,
                                       stc8h_u16 ticks,
                                       stc8h_u16 long_ticks,
                                       stc8h_u16 double_ticks)
{
    return update_many(button, 0u, ticks, long_ticks, double_ticks);
}

static void test_short_press_emits_after_double_window(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 5u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(update_many(&button, 0u, 29u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 0u, 500u, 30u) == APP_BUTTON_EVENT_SHORT);
    assert(app_button_update(&button, 0u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
}

static void test_double_press_emits_double_without_short(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 3u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(update_many(&button, 0u, 10u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(update_many(&button, 1u, 3u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 500u, 30u) == APP_BUTTON_EVENT_DOUBLE);
    assert(update_many(&button, 0u, 31u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
}

static void test_long_press_5s_emits_once_without_release(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 499u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 1u, 500u, 30u) == APP_BUTTON_EVENT_LONG);
    assert(update_many(&button, 1u, 20u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
}

static void test_long_press_3s_emits_once_without_release(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 299u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 1u, 300u, 0u) == APP_BUTTON_EVENT_LONG);
    assert(release_many(&button, 5u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
}

static void test_zero_double_window_emits_short_on_release(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 3u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 300u, 0u) == APP_BUTTON_EVENT_SHORT);
}

static void test_long_press_state_waits_for_release_across_mode_switch(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 499u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 1u, 500u, 30u) == APP_BUTTON_EVENT_LONG);
    assert(update_many(&button, 1u, 350u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
    assert(release_many(&button, 5u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
    assert(update_many(&button, 1u, 299u, 300u, 0u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 1u, 300u, 0u) == APP_BUTTON_EVENT_LONG);
}

static void test_long_press_tolerates_short_inactive_noise(void)
{
    app_button_t button;

    app_button_init(&button);

    assert(update_many(&button, 1u, 250u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 0u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(update_many(&button, 1u, 249u, 500u, 30u) == APP_BUTTON_EVENT_NONE);
    assert(app_button_update(&button, 1u, 500u, 30u) == APP_BUTTON_EVENT_LONG);
}

int main(void)
{
    test_short_press_emits_after_double_window();
    test_double_press_emits_double_without_short();
    test_long_press_5s_emits_once_without_release();
    test_long_press_3s_emits_once_without_release();
    test_zero_double_window_emits_short_on_release();
    test_long_press_state_waits_for_release_across_mode_switch();
    test_long_press_tolerates_short_inactive_noise();
    return 0;
}
