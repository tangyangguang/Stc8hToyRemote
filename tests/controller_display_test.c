#define APP_DISPLAY_ENABLE_LEGACY_CHANNEL 1
#include "app_display.h"

#include <assert.h>

static void assert_channel(stc8h_u8 prefix, stc8h_u8 channel)
{
    stc8h_u8 segments[4];

    app_display_prefixed_channel_segments(prefix, channel, segments);

    assert(segments[0] == prefix);
    assert(segments[1] == app_display_digit((stc8h_u8)(channel / 100u)));
    assert(segments[2] == app_display_digit((stc8h_u8)((channel / 10u) % 10u)));
    assert(segments[3] == app_display_digit((stc8h_u8)(channel % 10u)));
}

static void test_channel_display_uses_status_prefix_and_three_digits(void)
{
    assert_channel(APP_DISPLAY_C, 76u);
    assert_channel(APP_DISPLAY_L, 76u);
    assert_channel(APP_DISPLAY_S, 76u);
    assert_channel(APP_DISPLAY_F, 76u);
    assert_channel(APP_DISPLAY_H, 76u);
    assert_channel(APP_DISPLAY_S, 125u);
}

static void test_legacy_numeric_channel_display_still_works(void)
{
    stc8h_u8 segments[4];

    app_display_channel_segments(40u, 1u, segments);

    assert(segments[0] == APP_DISPLAY_BLANK);
    assert(segments[1] == APP_DISPLAY_COLON);
    assert(segments[2] == app_display_digit(4u));
    assert(segments[3] == app_display_digit(0u));
}

static void test_error_display_shows_e001(void)
{
    stc8h_u8 segments[4];

    app_display_error_segments(1u, segments);

    assert(segments[0] == APP_DISPLAY_E);
    assert(segments[1] == app_display_digit(0u));
    assert(segments[2] == app_display_digit(0u));
    assert(segments[3] == app_display_digit(1u));
}

static void test_decimal_helpers_format_without_losing_leading_zeroes(void)
{
    stc8h_u8 segments[4];

    app_display_set_2_digits(7u, &segments[2]);
    assert(segments[2] == app_display_digit(0u));
    assert(segments[3] == app_display_digit(7u));

    app_display_set_3_digits(76u, &segments[1]);
    assert(segments[1] == app_display_digit(0u));
    assert(segments[2] == app_display_digit(7u));
    assert(segments[3] == app_display_digit(6u));

    app_display_set_3_digits(125u, &segments[1]);
    assert(segments[1] == app_display_digit(1u));
    assert(segments[2] == app_display_digit(2u));
    assert(segments[3] == app_display_digit(5u));

    app_display_set_4_digits(9999u, segments);
    assert(segments[0] == app_display_digit(9u));
    assert(segments[1] == app_display_digit(9u));
    assert(segments[2] == app_display_digit(9u));
    assert(segments[3] == app_display_digit(9u));

    app_display_set_4_digits(10000u, segments);
    assert(segments[0] == app_display_digit(9u));
    assert(segments[1] == app_display_digit(9u));
    assert(segments[2] == app_display_digit(9u));
    assert(segments[3] == app_display_digit(9u));
}

static void test_config_display_shows_p_item_colon_value(void)
{
    stc8h_u8 segments[4];

    app_display_config_segments(1u, 1u, segments);
    assert(segments[0] == APP_DISPLAY_P);
    assert(segments[1] == (stc8h_u8)(app_display_digit(1u) | APP_DISPLAY_COLON));
    assert(segments[2] == APP_DISPLAY_BLANK);
    assert(segments[3] == app_display_digit(1u));

    app_display_config_segments(3u, 45u, segments);
    assert(segments[0] == APP_DISPLAY_P);
    assert(segments[1] == (stc8h_u8)(app_display_digit(3u) | APP_DISPLAY_COLON));
    assert(segments[2] == app_display_digit(4u));
    assert(segments[3] == app_display_digit(5u));

    app_display_config_segments(5u, 20u, segments);
    assert(segments[0] == APP_DISPLAY_P);
    assert(segments[1] == (stc8h_u8)(app_display_digit(5u) | APP_DISPLAY_COLON));
    assert(segments[2] == app_display_digit(2u));
    assert(segments[3] == app_display_digit(0u));
}

static void test_control_display_keeps_zero_speed_digits_while_braking(void)
{
    stc8h_u8 segments[4];

    app_display_control_segments(0u, 0u, 1u, segments);

    assert(segments[0] == APP_DISPLAY_DASH);
    assert(segments[1] == APP_DISPLAY_BLANK);
    assert(segments[2] == app_display_digit(0u));
    assert(segments[3] == app_display_digit(0u));
}

int main(void)
{
    test_channel_display_uses_status_prefix_and_three_digits();
    test_legacy_numeric_channel_display_still_works();
    test_error_display_shows_e001();
    test_decimal_helpers_format_without_losing_leading_zeroes();
    test_config_display_shows_p_item_colon_value();
    test_control_display_keeps_zero_speed_digits_while_braking();
    return 0;
}
