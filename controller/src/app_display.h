#ifndef CONTROLLER_APP_DISPLAY_H
#define CONTROLLER_APP_DISPLAY_H

#include "stc8h_config.h"

#define APP_DISPLAY_BLANK 0x00u
#define APP_DISPLAY_DASH 0x40u
#define APP_DISPLAY_A 0x77u
#define APP_DISPLAY_C 0x39u
#define APP_DISPLAY_E 0x79u
#define APP_DISPLAY_F 0x71u
#define APP_DISPLAY_H 0x76u
#define APP_DISPLAY_L 0x38u
#define APP_DISPLAY_P 0x73u
#define APP_DISPLAY_S 0x6Du
#define APP_DISPLAY_UP 0x23u
#define APP_DISPLAY_DOWN 0x1Cu
#define APP_DISPLAY_COLON 0x80u

#ifndef APP_DISPLAY_ENABLE_LEGACY_CHANNEL
#define APP_DISPLAY_ENABLE_LEGACY_CHANNEL 0
#endif

static stc8h_u8 app_display_digit(stc8h_u8 value)
{
    static STC8H_CODE stc8h_u8 table[10] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u,
        0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu
    };

    return table[value];
}

#define app_display_set_2_digits(value, segments) do { \
    stc8h_u8 app_display_v_ = (stc8h_u8)(value); \
    stc8h_u8 app_display_digit_ = 0u; \
    while (app_display_v_ >= 10u) { \
        app_display_v_ = (stc8h_u8)(app_display_v_ - 10u); \
        ++app_display_digit_; \
    } \
    (segments)[0] = app_display_digit(app_display_digit_); \
    (segments)[1] = app_display_digit(app_display_v_); \
} while (0)

#define app_display_set_3_digits(value, segments) do { \
    stc8h_u8 app_display_v3_ = (stc8h_u8)(value); \
    stc8h_u8 app_display_digit3_ = 0u; \
    while (app_display_v3_ >= 100u) { \
        app_display_v3_ = (stc8h_u8)(app_display_v3_ - 100u); \
        ++app_display_digit3_; \
    } \
    (segments)[0] = app_display_digit(app_display_digit3_); \
    app_display_set_2_digits(app_display_v3_, &(segments)[1]); \
} while (0)

#define app_display_set_4_digits(value, segments) do { \
    stc8h_u16 app_display_v4_ = (stc8h_u16)(value); \
    stc8h_u8 app_display_digit4_ = 0u; \
    if (app_display_v4_ > 9999u) { \
        app_display_v4_ = 9999u; \
    } \
    while (app_display_v4_ >= 1000u) { \
        app_display_v4_ = (stc8h_u16)(app_display_v4_ - 1000u); \
        ++app_display_digit4_; \
    } \
    (segments)[0] = app_display_digit(app_display_digit4_); \
    app_display_digit4_ = 0u; \
    while (app_display_v4_ >= 100u) { \
        app_display_v4_ = (stc8h_u16)(app_display_v4_ - 100u); \
        ++app_display_digit4_; \
    } \
    (segments)[1] = app_display_digit(app_display_digit4_); \
    app_display_set_2_digits((stc8h_u8)app_display_v4_, &(segments)[2]); \
} while (0)

#if APP_DISPLAY_ENABLE_LEGACY_CHANNEL
static void app_display_channel_segments(stc8h_u8 channel, stc8h_u8 colon, stc8h_u8 *segments)
{
    segments[0] = APP_DISPLAY_BLANK;
    if (channel >= 100u) {
        segments[1] = app_display_digit(1u);
        channel = (stc8h_u8)(channel - 100u);
    } else {
        segments[1] = APP_DISPLAY_BLANK;
    }
    app_display_set_2_digits(channel, &segments[2]);
    if (colon != 0u) {
        segments[1] |= APP_DISPLAY_COLON;
    }
}
#endif

#define app_display_prefixed_channel_segments(prefix, channel, segments) do { \
    (segments)[0] = (prefix); \
    app_display_set_3_digits((channel), &(segments)[1]); \
} while (0)

#define app_display_error_segments(code, segments) do { \
    (segments)[0] = APP_DISPLAY_E; \
    app_display_set_3_digits((code), &(segments)[1]); \
} while (0)

#define app_display_config_segments(item, value, segments) do { \
    (segments)[0] = APP_DISPLAY_P; \
    (segments)[1] = (stc8h_u8)(app_display_digit((item)) | APP_DISPLAY_COLON); \
    app_display_set_2_digits((value), &(segments)[2]); \
    if ((value) < 10u) { \
        (segments)[2] = APP_DISPLAY_BLANK; \
    } \
} while (0)

#endif
