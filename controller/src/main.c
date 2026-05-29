#include "app_config.h"
#define APP_BUTTON_RELEASE_TICKS 20u
#define APP_BUTTON_FIXED_LONG_TICKS 10000u
#include "app_button.h"
#include "app_display.h"
#include "app_input.h"
#include "app_radio.h"
#include "app_steering.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "drv_tm1637.h"
#include "proto_rf_link.h"
#include "stc8h_delay.h"
#include "stc8h_interrupt.h"
#include "stc8h_sfr.h"
#include "stc8h_spi.h"
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_CONTAINS 0
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_NEXT 0
#define TOY_REMOTE_CHANNEL_POOL_ENABLE_PREV 0
#include "toy_remote_channels.h"
#include "toy_remote_protocol.h"

#ifndef APP_INPUT_DIAG_DISPLAY
#define APP_INPUT_DIAG_DISPLAY 0
#endif

#ifndef APP_STARTUP_DISPLAY_TEST
#define APP_STARTUP_DISPLAY_TEST 0
#endif

#ifndef APP_DISABLE_RADIO
#define APP_DISABLE_RADIO 0
#endif

#ifndef APP_STATIC_DISPLAY_DIAG
#define APP_STATIC_DISPLAY_DIAG 0
#endif

#ifndef APP_FRAME_DISPLAY_DIAG
#define APP_FRAME_DISPLAY_DIAG 0
#endif

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA app_config_t config;
static STC8H_XDATA toy_remote_control_t control;
#if APP_STEERING_MIN_STEP_DEGREES > 1u
static stc8h_u8 steering_output_angle = TOY_REMOTE_STEERING_CENTER;
#endif
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
static stc8h_u8 display_segments[4];
static stc8h_u8 display_last_segments[4];
static stc8h_u8 display_dirty;
static STC8H_BIT rx_voltage_valid;
static stc8h_u16 rx_battery_centivolts;
static stc8h_u16 tx_battery_centivolts;
static stc8h_u8 voltage_display_start_128ms;
static stc8h_u8 show_rx_voltage;
static stc8h_u8 voltage_display_active;
static stc8h_u8 current_channel;
static stc8h_u8 radio_failures;
static stc8h_u8 app_state;
static stc8h_u8 link_warning;
static stc8h_u8 link_blink;
static stc8h_u8 scan_requested;
static app_button_t ec11_button;
static stc8h_u8 ec11_button_press_speed;
static stc8h_u16 ui_last_tick_half_ms;
#if APP_INPUT_DIAG_DISPLAY
static stc8h_u8 input_diag_blink;
static stc8h_u8 input_diag_delta_hold;
#endif
static stc8h_u8 config_mode;
static stc8h_u8 config_item;

#define APP_CONFIG_ITEM_DIRECTION_REVERSE 1u
#define APP_CONFIG_ITEM_STEERING_REVERSE 2u
#define APP_CONFIG_ITEM_STEERING_TRIM 3u
#define APP_CONFIG_ITEM_DEADBAND 4u
#define APP_CONFIG_ITEM_REDUCE 5u
#define APP_STATE_TRY_SAVED 0u
#define APP_STATE_CONNECTED 1u
#define APP_STATE_LOST 2u
#define APP_BUTTON_LONG_NORMAL_TICKS APP_BUTTON_FIXED_LONG_TICKS
#define APP_BUTTON_DOUBLE_TICKS 1000u
#define APP_SCAN_PROBE_PACKETS 2u
#define APP_SCAN_POOL_PASSES 1u
#define APP_RADIO_FAILURE_LIMIT 3u
#define APP_LINK_BLINK_TICKS 10u
#define APP_LOOP_INTERVAL_MS 20u
#define APP_UI_UPDATE_MS 10u
#define APP_UI_UPDATES_PER_LOOP (APP_LOOP_INTERVAL_MS / APP_UI_UPDATE_MS)
#define APP_VOLTAGE_DISPLAY_128MS_TICKS 12u
#define APP_VOLTAGE_LABEL_128MS_TICKS 3u
#define APP_TIMER0_ENCODER_RELOAD 0xFE33u
#define APP_AUXR_T0_1T 0x80u
#define APP_INTCLKO_T0CLKO 0x01u
#if APP_INPUT_DIAG_DISPLAY
#define APP_INPUT_DIAG_REFRESH_MS 5u
#define APP_INPUT_DIAG_BLINK_TICKS 6u
#define APP_INPUT_DIAG_DELTA_HOLD_TICKS 8u
#endif

static void display_commit_raw4(void)
{
    if ((display_dirty == 0u) &&
        (display_segments[0] == display_last_segments[0]) &&
        (display_segments[1] == display_last_segments[1]) &&
        (display_segments[2] == display_last_segments[2]) &&
        (display_segments[3] == display_last_segments[3])) {
        return;
    }

    (void)drv_tm1637_display_raw4_data(display_segments);

    display_last_segments[0] = display_segments[0];
    display_last_segments[1] = display_segments[1];
    display_last_segments[2] = display_segments[2];
    display_last_segments[3] = display_segments[3];
    display_dirty = 0u;
}

STC8H_INTERRUPT(timer0_isr, STC8H_VECTOR_TIMER0)
{
    TF0 = 0;
    app_input_encoder_tick_isr();
}

static void app_timer0_init_encoder_tick(void)
{
    TR0 = 0;
    ET0 = 0;
    TMOD &= (stc8h_u8)~0x0Fu;
    AUXR &= (stc8h_u8)~APP_AUXR_T0_1T;
    INTCLKO &= (stc8h_u8)~APP_INTCLKO_T0CLKO;
    TL0 = (stc8h_u8)APP_TIMER0_ENCODER_RELOAD;
    TH0 = (stc8h_u8)(APP_TIMER0_ENCODER_RELOAD >> 8);
    TF0 = 0;
    ET0 = 1;
    EA = 1;
    TR0 = 1;
}

static void display_init(void)
{
    P1M0 &= (stc8h_u8)~TOY_REMOTE_TX_TM1637_CLK_MASK;
    P1M1 &= (stc8h_u8)~TOY_REMOTE_TX_TM1637_CLK_MASK;
    P3M0 &= (stc8h_u8)~TOY_REMOTE_TX_TM1637_DIO_MASK;
    P3M1 &= (stc8h_u8)~TOY_REMOTE_TX_TM1637_DIO_MASK;
    P_SW2 |= 0x80u;
    P1IE |= TOY_REMOTE_TX_TM1637_CLK_MASK;
    P3IE |= TOY_REMOTE_TX_TM1637_DIO_MASK;
    P1PU |= TOY_REMOTE_TX_TM1637_CLK_MASK;
    P3PU |= TOY_REMOTE_TX_TM1637_DIO_MASK;
    drv_tm1637_init();
    display_last_segments[0] = 0xFFu;
    display_last_segments[1] = 0xFFu;
    display_last_segments[2] = 0xFFu;
    display_last_segments[3] = 0xFFu;
    display_dirty = 1u;
}

static void display_control(void)
{
    app_display_control_segments(control.direction, control.speed, control.brake, display_segments);

    ++link_blink;
    if (link_blink >= APP_LINK_BLINK_TICKS) {
        link_blink = 0u;
    }
    if ((link_warning != 0u) && (link_blink < (APP_LINK_BLINK_TICKS / 2u))) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    display_commit_raw4();
}

#if APP_STARTUP_DISPLAY_TEST
static void display_startup_self_test(void)
{
    stc8h_u8 digit;
    stc8h_u8 segment;

    for (digit = 0u; digit < 10u; ++digit) {
        segment = app_display_digit(digit);
        display_segments[0] = segment;
        display_segments[1] = segment;
        display_segments[2] = segment;
        display_segments[3] = segment;
        display_commit_raw4();
        stc8h_delay_ms(40u);
    }
}
#endif

#if APP_INPUT_DIAG_DISPLAY
static void display_input_diag(stc8h_s16 delta)
{
    stc8h_u8 brake_sources;

    if (delta != 0) {
        input_diag_delta_hold = APP_INPUT_DIAG_DELTA_HOLD_TICKS;
    } else if (input_diag_delta_hold != 0u) {
        --input_diag_delta_hold;
    }

    brake_sources = 0u;
    if (TOY_REMOTE_TX_EC11_SW_ACTIVE() != 0u) {
        brake_sources |= 1u;
    }
    if (TOY_REMOTE_TX_BRAKE_ACTIVE() != 0u) {
        brake_sources |= 2u;
    }

    if (control.brake != 0u) {
        display_segments[0] = APP_DISPLAY_DASH;
    } else {
        display_segments[0] = (TOY_REMOTE_TX_DIR_REVERSE() != 0u) ? APP_DISPLAY_DOWN : APP_DISPLAY_UP;
    }
    display_segments[1] = app_display_digit(brake_sources);
    if (control.speed >= 100u) {
        display_segments[2] = APP_DISPLAY_A;
        display_segments[3] = app_display_digit(0u);
    } else {
        app_display_set_2_digits(control.speed, &display_segments[2]);
    }

    ++input_diag_blink;
    if (input_diag_blink >= APP_INPUT_DIAG_BLINK_TICKS) {
        input_diag_blink = 0u;
    }
    if ((input_diag_delta_hold != 0u) || (input_diag_blink < (APP_INPUT_DIAG_BLINK_TICKS / 2u))) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }

    display_commit_raw4();
}
#endif

static void display_voltage(stc8h_u16 value)
{
    app_display_set_4_digits(value, display_segments);

    if (show_rx_voltage == 0u) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    display_commit_raw4();
}

static void display_config(void)
{
    stc8h_u8 value;

    if (config_item == APP_CONFIG_ITEM_DIRECTION_REVERSE) {
        value = ((config.flags & APP_CONFIG_FLAG_DIRECTION_REVERSE) != 0u) ? 1u : 0u;
    } else if (config_item == APP_CONFIG_ITEM_STEERING_REVERSE) {
        value = ((config.flags & APP_CONFIG_FLAG_STEERING_REVERSE) != 0u) ? 1u : 0u;
    } else if (config_item == APP_CONFIG_ITEM_STEERING_TRIM) {
        if (config.steering_trim < 0) {
            value = (stc8h_u8)(0 - config.steering_trim);
            display_segments[0] = APP_DISPLAY_P;
            display_segments[1] = APP_DISPLAY_DASH;
            app_display_set_2_digits(value, &display_segments[2]);
            display_commit_raw4();
            return;
        }
        value = (stc8h_u8)config.steering_trim;
    } else if (config_item == APP_CONFIG_ITEM_DEADBAND) {
        value = config.steering_deadband;
    } else {
        value = config.steering_reduce;
    }
    app_display_config_segments(config_item, value, display_segments);
    display_commit_raw4();
}

static void enter_config_mode(void)
{
    config_mode = 1u;
    app_input_set_speed_accel_enabled(0u);
    config_item = APP_CONFIG_ITEM_DIRECTION_REVERSE;
    control.speed = 0u;
    control.brake = 1u;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
    display_config();
}

static void config_set_draft_flag(stc8h_u8 mask, stc8h_u8 enabled)
{
    if (enabled != 0u) {
        config.flags |= mask;
    } else {
        config.flags &= (stc8h_u8)~mask;
    }
}

static void exit_config_mode_save(void)
{
    (void)app_config_save(&config);
    config_mode = 0u;
    app_state = APP_STATE_CONNECTED;
    app_input_set_speed_accel_enabled(1u);
}

static void handle_config_mode(stc8h_s16 delta, app_button_event_t button_event)
{
    stc8h_s16 value;

    control.speed = 0u;
    control.brake = 1u;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;

    if (button_event == APP_BUTTON_EVENT_SHORT) {
        ++config_item;
        if (config_item > APP_CONFIG_ITEM_REDUCE) {
            config_item = APP_CONFIG_ITEM_DIRECTION_REVERSE;
        }
    } else if (button_event == APP_BUTTON_EVENT_LONG) {
        exit_config_mode_save();
        return;
    }

    if (delta != 0) {
        if (config_item == APP_CONFIG_ITEM_DIRECTION_REVERSE) {
            config_set_draft_flag(APP_CONFIG_FLAG_DIRECTION_REVERSE, (delta > 0) ? 1u : 0u);
        } else if (config_item == APP_CONFIG_ITEM_STEERING_REVERSE) {
            config_set_draft_flag(APP_CONFIG_FLAG_STEERING_REVERSE, (delta > 0) ? 1u : 0u);
        } else if (config_item == APP_CONFIG_ITEM_STEERING_TRIM) {
            value = (stc8h_s16)((stc8h_s16)config.steering_trim + delta);
            if (value < APP_CONFIG_STEERING_TRIM_MIN) {
                value = APP_CONFIG_STEERING_TRIM_MIN;
            } else if (value > APP_CONFIG_STEERING_TRIM_MAX) {
                value = APP_CONFIG_STEERING_TRIM_MAX;
            }
            config.steering_trim = (stc8h_s8)value;
        } else if (config_item == APP_CONFIG_ITEM_REDUCE) {
            value = (stc8h_s16)((stc8h_s16)config.steering_reduce + delta);
            if (value < 0) {
                value = 0;
            } else if (value > APP_CONFIG_STEERING_REDUCE_MAX) {
                value = APP_CONFIG_STEERING_REDUCE_MAX;
            }
            config.steering_reduce = (stc8h_u8)value;
        } else {
            value = (stc8h_s16)((stc8h_s16)config.steering_deadband + delta);
            if (value < APP_CONFIG_STEERING_DEADBAND_MIN) {
                value = APP_CONFIG_STEERING_DEADBAND_MIN;
            } else if (value > APP_CONFIG_STEERING_DEADBAND_MAX) {
                value = APP_CONFIG_STEERING_DEADBAND_MAX;
            }
            config.steering_deadband = (stc8h_u8)value;
        }
    }

    display_config();
}

static void make_control_packet(void)
{
    stc8h_u8 direction;
    stc8h_u8 flags;
    stc8h_s8 steering_trim;
    stc8h_u8 steering_deadband;
    stc8h_u8 steering_reduce;
    stc8h_u8 steering_angle;

    flags = config.flags;
    steering_trim = config.steering_trim;
    steering_deadband = config.steering_deadband;
    steering_reduce = config.steering_reduce;

    control.tx_id = config.tx_id;
    direction = control.direction;
    if ((flags & APP_CONFIG_FLAG_DIRECTION_REVERSE) != 0u) {
        direction = (direction == 0u) ? 1u : 0u;
    }

    payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] = direction;
    payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] = control.speed;
    payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] = control.brake;
    steering_angle =
        app_steering_apply_config(control.steering_angle, flags, steering_trim, steering_deadband, steering_reduce);
#if APP_STEERING_MIN_STEP_DEGREES > 1u
    steering_angle = app_steering_apply_min_step(steering_output_angle, steering_angle);
    steering_output_angle = steering_angle;
#endif
    payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] = steering_angle;
    payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] = control.light;
    payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] = control.buzzer;
    payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] = control.aux_pwm;
    payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] = control.request_voltage;
    TOY_REMOTE_PUT_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L, control.tx_id);
    (void)proto_rf_link_send_data_fixed_xdata(&link, packet, payload);
}

static void display_state_channel(stc8h_u8 prefix)
{
    app_display_prefixed_channel_segments(prefix, current_channel, display_segments);
    display_commit_raw4();
}

static stc8h_u8 handle_ack_status(void)
{
    const stc8h_u8 *ack;
    const stc8h_u8 *body;

    if (app_radio_ack_len != APP_RADIO_STATUS_ACK_SIZE) {
        return 0u;
    }

    ack = app_radio_ack_packet;
    body = &ack[PROTO_RF_LINK_HEADER_SIZE];
    if ((ack[0] != PROTO_RF_LINK_MAGIC) ||
        (ack[1] != PROTO_RF_LINK_VERSION) ||
        (ack[2] != PROTO_RF_LINK_PACKET_STATUS) ||
        (ack[6] != 2u) ||
        (ack[7] != 1u) ||
        (ack[8] != TOY_REMOTE_STATUS_PAYLOAD_SIZE) ||
        (body[TOY_REMOTE_STATUS_OFFSET_TX_ID_L] != (stc8h_u8)config.tx_id) ||
        (body[TOY_REMOTE_STATUS_OFFSET_TX_ID_H] != (stc8h_u8)(config.tx_id >> 8))) {
        return 0u;
    }
    if ((body[TOY_REMOTE_STATUS_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (body[TOY_REMOTE_STATUS_OFFSET_LINK_STATE] > TOY_REMOTE_LINK_STATE_LOST) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] > TOY_REMOTE_VOLTAGE_INT_MAX) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] > TOY_REMOTE_VOLTAGE_DEC_MAX)) {
        return 0u;
    }

    if (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] != 0u) {
        rx_battery_centivolts =
            (stc8h_u16)((stc8h_u16)body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] * 100u +
                        body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC]);
        rx_voltage_valid = 1u;
    }
    return 1u;
}

static stc8h_u8 send_control_packet(void)
{
    app_radio_tx_result_t result;

    if (config_mode == 0u) {
        (void)app_input_update_speed(&control);
        app_input_update_discrete(&control);
    }
    app_input_update_steering(&control);
    make_control_packet();
    result = app_radio_send_packet_with_ack(packet);
    if (result == APP_RADIO_TX_ACK_PAYLOAD_OK) {
        if (handle_ack_status() != 0u) {
            return 1u;
        }
        return 2u;
    }
    return 0u;
}

static stc8h_u8 probe_current_channel(void)
{
    stc8h_u8 i;
    stc8h_u8 result;

    for (i = 0u; i < APP_SCAN_PROBE_PACKETS; ++i) {
        result = send_control_packet();
        if (result == 1u) {
            return 1u;
        }
        if (result == 2u) {
            stc8h_delay_ms(5u);
        }
    }
    return 0u;
}

static stc8h_u8 scan_one_channel(stc8h_u8 channel)
{
    app_radio_set_channel(channel);
    current_channel = channel;
    display_state_channel(APP_DISPLAY_S);
    if (probe_current_channel() != 0u) {
        if (config.last_channel != channel) {
            config.last_channel = channel;
            (void)app_config_save(&config);
        }
        display_state_channel(APP_DISPLAY_F);
        stc8h_delay_ms(750u);
        return 1u;
    }
    return 0u;
}

static void scan_channels(void)
{
    stc8h_u8 channel;
    stc8h_u8 index;
    stc8h_u8 pool_pass;

    while (1) {
        for (pool_pass = 0u; pool_pass < APP_SCAN_POOL_PASSES; ++pool_pass) {
            for (index = 0u; index < TOY_REMOTE_CHANNEL_POOL_COUNT; ++index) {
                if (scan_one_channel(toy_remote_channel_pool_value(index)) != 0u) {
                    return;
                }
            }
        }
        for (channel = 0u; channel <= 125u; ++channel) {
            if (scan_one_channel(channel) != 0u) {
                return;
            }
        }
    }
}

static void update_voltage_display(stc8h_u16 now_half_ms)
{
    stc8h_u8 now_128ms;
    stc8h_u8 elapsed_128ms;

    now_128ms = (stc8h_u8)(now_half_ms >> 8);

    if (voltage_display_active == 0u) {
        voltage_display_active = 1u;
        voltage_display_start_128ms = now_128ms;
        show_rx_voltage = 0u;
        tx_battery_centivolts = app_input_read_tx_battery_centivolts();
    }

    elapsed_128ms = (stc8h_u8)(now_128ms - voltage_display_start_128ms);
    if (elapsed_128ms >= APP_VOLTAGE_DISPLAY_128MS_TICKS) {
        voltage_display_start_128ms = now_128ms;
        elapsed_128ms = 0u;
        if (show_rx_voltage != 0u) {
            show_rx_voltage = 0u;
            tx_battery_centivolts = app_input_read_tx_battery_centivolts();
        } else {
            show_rx_voltage = 1u;
        }
    }

    if (elapsed_128ms < APP_VOLTAGE_LABEL_128MS_TICKS) {
        app_display_source_segments(
            (show_rx_voltage == 0u) ? APP_DISPLAY_C :
            ((app_state == APP_STATE_CONNECTED) ? APP_DISPLAY_R : APP_DISPLAY_H),
            display_segments);
        display_commit_raw4();
    } else if (show_rx_voltage != 0u) {
        if (rx_voltage_valid == 0u) {
            app_display_source_segments(APP_DISPLAY_DASH, display_segments);
            display_commit_raw4();
        } else {
            display_voltage(rx_battery_centivolts);
        }
    } else {
        display_voltage(tx_battery_centivolts);
    }
}

static void run_ui_slice(void)
{
    stc8h_s16 delta;
#if !APP_INPUT_DIAG_DISPLAY
    app_button_event_t button_event;
    stc8h_u8 ec11_active;
    stc8h_u16 ui_tick_half_ms;
    stc8h_u16 elapsed_ticks;
#endif

#if !APP_INPUT_DIAG_DISPLAY
    ui_tick_half_ms = app_input_tick_half_ms();
    elapsed_ticks = (stc8h_u16)(ui_tick_half_ms - ui_last_tick_half_ms);
    ui_last_tick_half_ms = ui_tick_half_ms;
    ec11_active = TOY_REMOTE_TX_EC11_SW_ACTIVE();
    if ((config_mode == 0u) && (ec11_active != 0u) && (ec11_button.was_active == 0u)) {
        ec11_button_press_speed = control.speed;
    }
#endif
    delta = app_input_update(&control);
#if APP_INPUT_DIAG_DISPLAY
    display_input_diag(delta);
#else
    button_event = app_button_update_elapsed(&ec11_button,
                                             ec11_active,
                                             elapsed_ticks,
                                             (config_mode != 0u) ? 0u : APP_BUTTON_DOUBLE_TICKS);
    if (config_mode != 0u) {
        handle_config_mode(delta, button_event);
    } else {
        if ((button_event == APP_BUTTON_EVENT_LONG) &&
            (ec11_button_press_speed == 0u)) {
            enter_config_mode();
        } else if ((button_event == APP_BUTTON_EVENT_DOUBLE) && (app_state == APP_STATE_LOST)) {
            scan_requested = 1u;
        }

        if (control.request_voltage != 0u) {
            update_voltage_display(ui_tick_half_ms);
        } else {
            voltage_display_active = 0u;
            if (app_state == APP_STATE_TRY_SAVED) {
                display_state_channel(APP_DISPLAY_C);
            } else if (app_state == APP_STATE_LOST) {
                display_state_channel(APP_DISPLAY_L);
            } else if (app_state == APP_STATE_CONNECTED) {
                display_control();
            }
        }
    }
#endif
}

#if APP_STATIC_DISPLAY_DIAG
static void display_static_patterns(void)
{
    static const STC8H_CODE stc8h_u8 patterns[][4] = {
        {0x06u, 0x06u, 0x00u, 0x00u}, /* left  11 */
        {0x06u, 0x5Bu, 0x00u, 0x00u}, /* left  12 */
        {0x06u, 0x6Du, 0x00u, 0x00u}, /* left  15 */
        {0x3Fu, 0x7Du, 0x00u, 0x00u}, /* left  06 */
        {0x7Fu, 0x7Fu, 0x00u, 0x00u}, /* left  88 */
        {0x00u, 0x00u, 0x06u, 0x06u}, /* right 11 */
        {0x00u, 0x00u, 0x06u, 0x5Bu}, /* right 12 */
        {0x00u, 0x00u, 0x06u, 0x6Du}, /* right 15 */
        {0x00u, 0x00u, 0x3Fu, 0x7Du}, /* right 06 */
        {0x00u, 0x00u, 0x7Fu, 0x7Fu}  /* right 88 */
    };
    stc8h_u8 i;

    while (1) {
        for (i = 0u; i < (stc8h_u8)(sizeof(patterns) / sizeof(patterns[0])); ++i) {
            display_segments[0] = patterns[i][0];
            display_segments[1] = patterns[i][1];
            display_segments[2] = patterns[i][2];
            display_segments[3] = patterns[i][3];
            display_dirty = 1u;
            display_commit_raw4();
            stc8h_delay_ms(2000u);
        }
    }
}
#endif

#if APP_FRAME_DISPLAY_DIAG
static void display_frame_patterns(void)
{
    static const STC8H_CODE stc8h_u8 patterns[][4] = {
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x3Fu, 0x3Fu}, /* ^:00 */
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x3Fu, 0x7Du}, /* ^:06 */
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x06u, 0x06u}, /* ^:11 */
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x06u, 0x5Bu}, /* ^:12 */
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x06u, 0x4Fu}, /* ^:13 */
        {APP_DISPLAY_UP, APP_DISPLAY_COLON, 0x06u, 0x6Du}  /* ^:15 */
    };
    stc8h_u8 i;

    while (1) {
        for (i = 0u; i < (stc8h_u8)(sizeof(patterns) / sizeof(patterns[0])); ++i) {
            display_segments[0] = patterns[i][0];
            display_segments[1] = patterns[i][1];
            display_segments[2] = patterns[i][2];
            display_segments[3] = patterns[i][3];
            display_dirty = 1u;
            display_commit_raw4();
            stc8h_delay_ms(2000u);
        }
    }
}
#endif

void main(void)
{
#if !APP_DISABLE_RADIO
    stc8h_u8 i;
#endif

    drv_nrf24l01_init_pins();
    stc8h_spi_init();
    proto_rf_link_init_xdata(&link);
    proto_rf_link_set_ids_xdata(&link, 1u, 2u);
    app_input_init(&control);
    app_button_init(&ec11_button);
    app_timer0_init_encoder_tick();
    if (app_config_load(&config) != STC8H_OK) {
        (void)app_config_save(&config);
    }
    control.tx_id = config.tx_id;
    current_channel = config.last_channel;
    display_init();
    display_state_channel(APP_DISPLAY_C);
    stc8h_delay_ms(150u);
#if APP_STARTUP_DISPLAY_TEST
    display_startup_self_test();
#endif
#if APP_STATIC_DISPLAY_DIAG
    display_static_patterns();
#endif
#if APP_FRAME_DISPLAY_DIAG
    display_frame_patterns();
#endif
    app_state = APP_STATE_TRY_SAVED;
    tx_battery_centivolts = app_input_read_tx_battery_centivolts();

#if APP_DISABLE_RADIO
    while (1) {
        run_ui_slice();
        stc8h_delay_ms(APP_UI_UPDATE_MS);
    }
#else
    if (app_radio_init_tx(current_channel) != STC8H_OK) {
        display_segments[0] = APP_DISPLAY_E;
        display_segments[1] = app_display_digit(0u);
        display_segments[2] = app_display_digit(0u);
        display_segments[3] = app_display_digit(1u);
        display_commit_raw4();
        while (1) {
            (void)app_input_update(&control);
#if APP_INPUT_DIAG_DISPLAY
            display_input_diag(0);
            stc8h_delay_ms(APP_INPUT_DIAG_REFRESH_MS);
#else
            stc8h_delay_ms(APP_UI_UPDATE_MS);
#endif
        }
    }

    while (1) {
        i = send_control_packet();

        if (i == 1u) {
            radio_failures = 0u;
            link_warning = 0u;
            if (config_mode == 0u) {
                app_state = APP_STATE_CONNECTED;
            }
        } else if (radio_failures < APP_RADIO_FAILURE_LIMIT) {
            ++radio_failures;
            link_warning = 1u;
        } else {
            app_state = APP_STATE_LOST;
            link_warning = 1u;
        }

        for (i = 0u; i < APP_UI_UPDATES_PER_LOOP; ++i) {
            run_ui_slice();
            stc8h_delay_ms(APP_UI_UPDATE_MS);
        }
        if ((scan_requested != 0u) && (app_state == APP_STATE_LOST)) {
            scan_requested = 0u;
            scan_channels();
            radio_failures = 0u;
            link_warning = 0u;
            app_state = APP_STATE_CONNECTED;
        }
    }
#endif
}
