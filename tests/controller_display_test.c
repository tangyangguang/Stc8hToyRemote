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
}

int main(void)
{
    test_channel_display_uses_status_prefix_and_three_digits();
    test_legacy_numeric_channel_display_still_works();
    test_error_display_shows_e001();
    test_config_display_shows_p_item_colon_value();
    return 0;
}
