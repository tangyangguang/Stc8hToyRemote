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

static stc8h_u8 app_display_digit(stc8h_u8 value)
{
    static STC8H_CODE stc8h_u8 table[10] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u,
        0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu
    };

    return table[value];
}

static void app_display_channel_segments(stc8h_u8 channel, stc8h_u8 colon, stc8h_u8 *segments)
{
    segments[0] = APP_DISPLAY_BLANK;
    segments[1] = (channel >= 100u) ? app_display_digit(1u) : APP_DISPLAY_BLANK;
    segments[2] = app_display_digit((channel >= 100u) ? (stc8h_u8)((channel - 100u) / 10u) : (stc8h_u8)(channel / 10u));
    segments[3] = app_display_digit((stc8h_u8)(channel % 10u));
    if (colon != 0u) {
        segments[1] |= APP_DISPLAY_COLON;
    }
}

#define app_display_set_3_digits(value, segments) do { \
    (segments)[1] = app_display_digit((stc8h_u8)((value) / 100u)); \
    (segments)[2] = app_display_digit((stc8h_u8)(((value) / 10u) % 10u)); \
    (segments)[3] = app_display_digit((stc8h_u8)((value) % 10u)); \
} while (0)

#define app_display_prefixed_channel_segments(prefix, channel, segments) do { \
    (segments)[0] = (prefix); \
    app_display_set_3_digits((channel), (segments)); \
} while (0)

#define app_display_error_segments(code, segments) do { \
    (segments)[0] = APP_DISPLAY_E; \
    app_display_set_3_digits((code), (segments)); \
} while (0)

#define app_display_config_segments(item, value, segments) do { \
    (segments)[0] = APP_DISPLAY_P; \
    (segments)[1] = (stc8h_u8)(app_display_digit((item)) | APP_DISPLAY_COLON); \
    (segments)[2] = ((value) < 10u) ? APP_DISPLAY_BLANK : app_display_digit((stc8h_u8)((value) / 10u)); \
    (segments)[3] = app_display_digit((stc8h_u8)((value) % 10u)); \
} while (0)

#endif
