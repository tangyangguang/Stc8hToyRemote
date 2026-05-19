#include "app_display.h"

#include <assert.h>

static void test_channel_display_shows_two_digit_channel_with_colon(void)
{
    stc8h_u8 segments[4];

    app_display_channel_segments(40u, 1u, segments);

    assert(segments[0] == APP_DISPLAY_BLANK);
    assert(segments[1] == APP_DISPLAY_COLON);
    assert(segments[2] == app_display_digit(4u));
    assert(segments[3] == app_display_digit(0u));
}

static void test_channel_display_shows_three_digit_channel_with_colon(void)
{
    stc8h_u8 segments[4];

    app_display_channel_segments(125u, 1u, segments);

    assert(segments[0] == APP_DISPLAY_BLANK);
    assert(segments[1] == (stc8h_u8)(app_display_digit(1u) | APP_DISPLAY_COLON));
    assert(segments[2] == app_display_digit(2u));
    assert(segments[3] == app_display_digit(5u));
}

int main(void)
{
    test_channel_display_shows_two_digit_channel_with_colon();
    test_channel_display_shows_three_digit_channel_with_colon();
    return 0;
}
