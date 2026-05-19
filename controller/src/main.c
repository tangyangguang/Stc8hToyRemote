#include "app_config.h"
#include "app_input.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_tm1637.h"
#include "proto_rf_link.h"
#include "stc8h_delay.h"
#include "stc8h_interrupt.h"
#include "stc8h_sfr.h"
#include "stc8h_spi.h"
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
static STC8H_XDATA toy_remote_status_t rx_status;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
static STC8H_XDATA stc8h_u8 display_segments[4];
static STC8H_XDATA stc8h_u8 display_last_segments[4];
static stc8h_u8 display_dirty;
static app_radio_tx_result_t tx_result;
static stc8h_u16 tx_battery_centivolts;
static stc8h_u8 voltage_display_divider;
static stc8h_u8 show_rx_voltage;
static stc8h_u8 current_channel;
static stc8h_u8 radio_failures;
#if APP_INPUT_DIAG_DISPLAY
static stc8h_u8 input_diag_blink;
static stc8h_u8 input_diag_delta_hold;
static stc8h_u8 input_diag_delta_segment;
#endif
static STC8H_XDATA stc8h_u8 config_mode;
static STC8H_XDATA stc8h_u16 config_hold_ticks;
static STC8H_XDATA stc8h_u8 config_wait_release;
static STC8H_XDATA stc8h_u8 config_item;
static STC8H_XDATA stc8h_u8 config_brake_prev;
static STC8H_XDATA stc8h_u8 config_ec11_prev;
static STC8H_XDATA stc8h_u8 config_buzzer_prev;

#define APP_DISPLAY_BLANK 0x00u
#define APP_DISPLAY_DASH 0x40u
#define APP_DISPLAY_A 0x77u
#define APP_DISPLAY_UP 0x23u
#define APP_DISPLAY_DOWN 0x1Cu
#define APP_DISPLAY_COLON 0x80u
#define APP_CONFIG_ENTER_TICKS 300u
#define APP_CONFIG_ITEM_REDUCE 0u
#define APP_CONFIG_ITEM_MIDDLE 1u
#define APP_LOOP_INTERVAL_MS 50u
#define APP_UI_UPDATE_MS 10u
#define APP_UI_UPDATES_PER_LOOP (APP_LOOP_INTERVAL_MS / APP_UI_UPDATE_MS)
#define APP_TIMER0_ENCODER_RELOAD 0xFE33u
#define APP_AUXR_T0_1T 0x80u
#define APP_INTCLKO_T0CLKO 0x01u
#if APP_INPUT_DIAG_DISPLAY
#define APP_INPUT_DIAG_REFRESH_MS 5u
#define APP_INPUT_DIAG_BLINK_TICKS 6u
#define APP_INPUT_DIAG_DELTA_HOLD_TICKS 8u
#endif

static stc8h_u8 display_digit(stc8h_u8 value)
{
    static const STC8H_CODE stc8h_u8 table[10] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u,
        0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu
    };

    return table[value];
}

static void display_commit_raw4(void)
{
    stc8h_u8 was_enabled;

    if ((display_dirty == 0u) &&
        (display_segments[0] == display_last_segments[0]) &&
        (display_segments[1] == display_last_segments[1]) &&
        (display_segments[2] == display_last_segments[2]) &&
        (display_segments[3] == display_last_segments[3])) {
        return;
    }

    was_enabled = (EA != 0u) ? 1u : 0u;
    EA = 0;
    (void)drv_tm1637_display_raw4(display_segments);
    if (was_enabled != 0u) {
        EA = 1;
    }

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

#define display_speed_tens(speed) (((speed) >= 100u) ? APP_DISPLAY_A : display_digit((stc8h_u8)((speed) / 10u)))

static void display_control(void)
{
    display_segments[1] = APP_DISPLAY_BLANK;
    if (control.brake != 0u) {
        display_segments[0] = APP_DISPLAY_DASH;
    } else {
        display_segments[0] = (control.direction == TOY_REMOTE_DIRECTION_REVERSE) ? APP_DISPLAY_DOWN : APP_DISPLAY_UP;
    }
    if ((control.brake != 0u) && (control.speed == 0u)) {
        display_segments[2] = APP_DISPLAY_DASH;
        display_segments[3] = APP_DISPLAY_DASH;
    } else {
        display_segments[2] = display_speed_tens(control.speed);
        display_segments[3] = display_digit((stc8h_u8)(control.speed % 10u));
    }

    if (tx_result != APP_RADIO_TX_DONE) {
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
        segment = display_digit(digit);
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
    if (delta > 0) {
        input_diag_delta_segment = APP_DISPLAY_UP;
        input_diag_delta_hold = APP_INPUT_DIAG_DELTA_HOLD_TICKS;
    } else if (delta < 0) {
        input_diag_delta_segment = APP_DISPLAY_DOWN;
        input_diag_delta_hold = APP_INPUT_DIAG_DELTA_HOLD_TICKS;
    } else if (input_diag_delta_hold != 0u) {
        --input_diag_delta_hold;
    } else {
        input_diag_delta_segment = APP_DISPLAY_BLANK;
    }

    display_segments[0] = display_digit(TOY_REMOTE_TX_EC11_A_READ());
    display_segments[1] = display_digit(TOY_REMOTE_TX_EC11_B_READ());
    display_segments[2] = display_digit(TOY_REMOTE_TX_EC11_SW_ACTIVE());
    display_segments[3] = (input_diag_delta_hold != 0u) ? input_diag_delta_segment : APP_DISPLAY_BLANK;

    ++input_diag_blink;
    if (input_diag_blink >= APP_INPUT_DIAG_BLINK_TICKS) {
        input_diag_blink = 0u;
    }
    if (input_diag_blink < (APP_INPUT_DIAG_BLINK_TICKS / 2u)) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }

    display_commit_raw4();
}
#endif

static void display_voltage(stc8h_u16 value)
{
    stc8h_u8 i;
    stc8h_u16 q;

    if (value > 9999u) {
        value = 9999u;
    }

    for (i = 4u; i != 0u; --i) {
        q = (stc8h_u16)(value / 10u);
        display_segments[i - 1u] = display_digit((stc8h_u8)(value - (stc8h_u16)(q * 10u)));
        value = q;
    }

    if (show_rx_voltage == 0u) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    display_commit_raw4();
}

static void display_config(void)
{
    display_segments[0] = display_digit((stc8h_u8)(config.steering_reduce / 10u));
    display_segments[1] = display_digit((stc8h_u8)(config.steering_reduce % 10u));
    display_segments[2] = display_digit((stc8h_u8)(config.steering_middle / 10u));
    display_segments[3] = display_digit((stc8h_u8)(config.steering_middle % 10u));
    if ((config.flags & APP_CONFIG_FLAG_STEERING_REVERSE) != 0u) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    if (config_item == APP_CONFIG_ITEM_MIDDLE) {
        display_segments[2] |= APP_DISPLAY_COLON;
    }
    display_commit_raw4();
}

static void enter_config_mode(void)
{
    config_mode = 1u;
    app_input_set_speed_accel_enabled(0u);
    config_wait_release = 1u;
    config_item = APP_CONFIG_ITEM_REDUCE;
    config_brake_prev = 1u;
    config_ec11_prev = 1u;
    config_buzzer_prev = TOY_REMOTE_TX_BUZZER_ACTIVE();
    control.speed = 0u;
    control.brake = 1u;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
}

static void config_set_flag(stc8h_u8 mask, stc8h_u8 enabled)
{
    stc8h_u8 old_flags;

    old_flags = config.flags;
    if (enabled != 0u) {
        config.flags |= mask;
    } else {
        config.flags &= (stc8h_u8)~mask;
    }
    if (config.flags != old_flags) {
        (void)app_config_save(&config);
    }
}

static void handle_config_mode(stc8h_s16 delta)
{
    stc8h_u8 brake_now;
    stc8h_u8 ec11_now;
    stc8h_u8 buzzer_now;
    stc8h_s16 value;

    control.speed = 0u;
    control.brake = 1u;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
    config_set_flag(APP_CONFIG_FLAG_DIRECTION_REVERSE, TOY_REMOTE_TX_DIR_REVERSE());

    brake_now = TOY_REMOTE_TX_BRAKE_ACTIVE();
    ec11_now = TOY_REMOTE_TX_EC11_SW_ACTIVE();
    buzzer_now = TOY_REMOTE_TX_BUZZER_ACTIVE();
    if (config_wait_release != 0u) {
        if ((brake_now == 0u) && (ec11_now == 0u)) {
            config_wait_release = 0u;
        }
    } else {
        if ((brake_now != 0u) && (config_brake_prev == 0u)) {
            config_set_flag(APP_CONFIG_FLAG_STEERING_REVERSE,
                            (config.flags & APP_CONFIG_FLAG_STEERING_REVERSE) == 0u);
        }
        if ((ec11_now != 0u) && (config_ec11_prev == 0u)) {
            config_item = (config_item == APP_CONFIG_ITEM_REDUCE) ? APP_CONFIG_ITEM_MIDDLE : APP_CONFIG_ITEM_REDUCE;
        }
        if ((buzzer_now != 0u) && (config_buzzer_prev == 0u)) {
            config_mode = 0u;
            app_input_set_speed_accel_enabled(1u);
            config_hold_ticks = 0u;
        }
    }

    if (delta != 0) {
        if (config_item == APP_CONFIG_ITEM_REDUCE) {
            value = (stc8h_s16)((stc8h_s16)config.steering_reduce + delta);
            if (value < 0) {
                value = 0;
            } else if (value > APP_CONFIG_STEERING_REDUCE_MAX) {
                value = APP_CONFIG_STEERING_REDUCE_MAX;
            }
            if (config.steering_reduce != (stc8h_u8)value) {
                config.steering_reduce = (stc8h_u8)value;
                (void)app_config_save(&config);
            }
        } else {
            value = (stc8h_s16)((stc8h_s16)config.steering_middle + delta);
            if (value < APP_CONFIG_STEERING_MIDDLE_MIN) {
                value = APP_CONFIG_STEERING_MIDDLE_MIN;
            } else if (value > APP_CONFIG_STEERING_MIDDLE_MAX) {
                value = APP_CONFIG_STEERING_MIDDLE_MAX;
            }
            if (config.steering_middle != (stc8h_u8)value) {
                config.steering_middle = (stc8h_u8)value;
                (void)app_config_save(&config);
            }
        }
    }

    config_brake_prev = brake_now;
    config_ec11_prev = ec11_now;
    config_buzzer_prev = buzzer_now;
    display_config();
}

static void update_config_entry(void)
{
    if ((TOY_REMOTE_TX_EC11_SW_ACTIVE() != 0u) && (TOY_REMOTE_TX_BRAKE_ACTIVE() != 0u)) {
        if (config_hold_ticks < APP_CONFIG_ENTER_TICKS) {
            ++config_hold_ticks;
        } else {
            enter_config_mode();
        }
    } else {
        config_hold_ticks = 0u;
    }
}

static void make_control_packet(void)
{
    stc8h_s16 angle;
    stc8h_u8 direction;

    control.tx_id = config.tx_id;
    direction = control.direction;
    if ((config.flags & APP_CONFIG_FLAG_DIRECTION_REVERSE) != 0u) {
        direction = (direction == 0u) ? 1u : 0u;
    }
    angle = (stc8h_s16)control.steering_angle;
    if ((config.flags & APP_CONFIG_FLAG_STEERING_REVERSE) != 0u) {
        angle = (stc8h_s16)(180 - angle);
    }
    angle = (stc8h_s16)(angle + ((stc8h_s16)config.steering_middle << 1) - 90);
    if (angle < (stc8h_s16)config.steering_reduce) {
        angle = (stc8h_s16)config.steering_reduce;
    } else if (angle > (stc8h_s16)(180 - config.steering_reduce)) {
        angle = (stc8h_s16)(180 - config.steering_reduce);
    }

    payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] = direction;
    payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] = control.speed;
    payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] = control.brake;
    payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] = (stc8h_u8)angle;
    payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] = control.light;
    payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] = control.buzzer;
    payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] = control.aux_pwm;
    payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] = control.request_voltage;
    TOY_REMOTE_PUT_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L, control.tx_id);
    (void)proto_rf_link_send_data_fixed(&link, packet, payload);
}

static void handle_ack_status(void)
{
    const stc8h_u8 *ack;
    const stc8h_u8 *body;

    if (app_radio_ack_len != APP_RADIO_PACKET_SIZE) {
        return;
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
        return;
    }
    if ((body[TOY_REMOTE_STATUS_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (body[TOY_REMOTE_STATUS_OFFSET_LINK_STATE] > TOY_REMOTE_LINK_STATE_LOST) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] > TOY_REMOTE_VOLTAGE_INT_MAX) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] > TOY_REMOTE_VOLTAGE_DEC_MAX)) {
        return;
    }

    rx_status.link_state = body[TOY_REMOTE_STATUS_OFFSET_LINK_STATE];
    rx_status.voltage_int = body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT];
    rx_status.voltage_dec = body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC];
    rx_status.tx_id = config.tx_id;
}

static stc8h_u8 probe_current_channel(void)
{
    stc8h_u8 i;

    for (i = 0u; i < 2u; ++i) {
        make_control_packet();
        tx_result = app_radio_send_packet_with_ack(packet);
        handle_ack_status();
        if (rx_status.tx_id == config.tx_id) {
            return 1u;
        }
        stc8h_delay_ms(5u);
    }
    return 0u;
}

static void scan_channels(void)
{
    stc8h_u8 channel;
    stc8h_u8 old_channel;

    old_channel = current_channel;
    if (probe_current_channel() != 0u) {
        return;
    }

    for (channel = 0u; channel <= 125u; ++channel) {
        app_radio_set_channel(channel);
        current_channel = channel;
        rx_status.tx_id = 0u;
        if (probe_current_channel() != 0u) {
            if (config.last_channel != channel) {
                config.last_channel = channel;
                (void)app_config_save(&config);
            }
            return;
        }
    }

    current_channel = old_channel;
    app_radio_set_channel(current_channel);
}

static void update_voltage_display(void)
{
    if (control.request_voltage == 0u) {
        voltage_display_divider = 0u;
        show_rx_voltage = 0u;
        return;
    }

    ++voltage_display_divider;
    if (voltage_display_divider >= 10u) {
        voltage_display_divider = 0u;
        show_rx_voltage = (show_rx_voltage == 0u) ? 1u : 0u;
        if (show_rx_voltage == 0u) {
            tx_battery_centivolts = app_input_read_tx_battery_centivolts();
        }
    }

    if (show_rx_voltage != 0u) {
        display_voltage((stc8h_u16)((stc8h_u16)rx_status.voltage_int * 100u + rx_status.voltage_dec));
    } else {
        display_voltage(tx_battery_centivolts);
    }
}

static void run_ui_slice(void)
{
    stc8h_s16 delta;

    delta = app_input_update(&control);
    if (config_mode != 0u) {
        handle_config_mode(delta);
    } else {
        update_config_entry();
        if (control.request_voltage != 0u) {
            update_voltage_display();
        } else {
            display_control();
        }
    }
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
    stc8h_s16 delta;
    stc8h_u8 i;
#endif

    stc8h_spi_init();
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 1u, 2u);
    app_input_init(&control);
    app_timer0_init_encoder_tick();
    if (app_config_load(&config) != STC8H_OK) {
        (void)app_config_save(&config);
    }
    control.tx_id = config.tx_id;
    current_channel = config.last_channel;
    display_init();
#if APP_STARTUP_DISPLAY_TEST
    display_startup_self_test();
#endif
#if APP_STATIC_DISPLAY_DIAG
    display_static_patterns();
#endif
#if APP_FRAME_DISPLAY_DIAG
    display_frame_patterns();
#endif
    rx_status.link_state = TOY_REMOTE_LINK_STATE_LOST;
    rx_status.voltage_int = 0u;
    rx_status.voltage_dec = 0u;
    rx_status.tx_id = 0u;
    tx_result = APP_RADIO_TX_ERROR;
    display_control();
    tx_battery_centivolts = app_input_read_tx_battery_centivolts();

#if APP_DISABLE_RADIO
    tx_result = APP_RADIO_TX_DONE;
    while (1) {
        run_ui_slice();
        stc8h_delay_ms(APP_UI_UPDATE_MS);
    }
#else
    if (app_radio_init_tx(current_channel) != STC8H_OK) {
        tx_result = APP_RADIO_TX_ERROR;
        while (1) {
            delta = app_input_update(&control);
#if APP_INPUT_DIAG_DISPLAY
            display_input_diag(delta);
            stc8h_delay_ms(APP_INPUT_DIAG_REFRESH_MS);
#else
            display_control();
            stc8h_delay_ms(APP_UI_UPDATE_MS);
#endif
        }
    }
    scan_channels();

    while (1) {
        make_control_packet();
        tx_result = app_radio_send_packet_with_ack(packet);
        handle_ack_status();

        if (tx_result == APP_RADIO_TX_DONE) {
            radio_failures = 0u;
        } else if (radio_failures < 10u) {
            ++radio_failures;
        } else {
            radio_failures = 0u;
            scan_channels();
        }

        for (i = 0u; i < APP_UI_UPDATES_PER_LOOP; ++i) {
            run_ui_slice();
            stc8h_delay_ms(APP_UI_UPDATE_MS);
        }
    }
#endif
}
