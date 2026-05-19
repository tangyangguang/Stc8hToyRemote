#include "app_indicator.h"

#include <assert.h>

static void run_boot_complete(app_indicator_t *indicator)
{
    assert(app_indicator_update(indicator, 100u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(indicator, 200u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(indicator, 300u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(indicator, 400u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(indicator, 500u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(indicator, 600u) == APP_INDICATOR_LED_ON);
}

static void test_boot_flashes_three_times_then_waits(void)
{
    app_indicator_t indicator;

    app_indicator_init(&indicator, 0u);

    assert(app_indicator_update(&indicator, 0u) == APP_INDICATOR_LED_ON);
    run_boot_complete(&indicator);
    assert(app_indicator_update(&indicator, 1099u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(&indicator, 1100u) == APP_INDICATOR_LED_OFF);
}

static void test_connected_waits_for_boot_then_stays_on(void)
{
    app_indicator_t indicator;

    app_indicator_init(&indicator, 0u);
    app_indicator_set_state(&indicator, APP_INDICATOR_STATE_CONNECTED, 10u);

    run_boot_complete(&indicator);
    assert(app_indicator_update(&indicator, 5000u) == APP_INDICATOR_LED_ON);
}

static void test_connecting_flashes_fast_after_boot(void)
{
    app_indicator_t indicator;

    app_indicator_init(&indicator, 0u);
    app_indicator_set_state(&indicator, APP_INDICATOR_STATE_CONNECTING, 0u);
    run_boot_complete(&indicator);

    assert(app_indicator_update(&indicator, 699u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(&indicator, 700u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(&indicator, 800u) == APP_INDICATOR_LED_ON);
}

static void test_radio_error_double_flashes_after_boot(void)
{
    app_indicator_t indicator;

    app_indicator_init(&indicator, 0u);
    app_indicator_set_state(&indicator, APP_INDICATOR_STATE_RADIO_ERROR, 0u);
    run_boot_complete(&indicator);

    assert(app_indicator_update(&indicator, 699u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(&indicator, 700u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(&indicator, 800u) == APP_INDICATOR_LED_ON);
    assert(app_indicator_update(&indicator, 900u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(&indicator, 1599u) == APP_INDICATOR_LED_OFF);
    assert(app_indicator_update(&indicator, 1600u) == APP_INDICATOR_LED_ON);
}

int main(void)
{
    test_boot_flashes_three_times_then_waits();
    test_connected_waits_for_boot_then_stays_on();
    test_connecting_flashes_fast_after_boot();
    test_radio_error_double_flashes_after_boot();
    return 0;
}
