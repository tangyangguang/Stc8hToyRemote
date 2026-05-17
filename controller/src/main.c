#include "app_config.h"
#include "app_input.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_tm1637.h"
#include "proto_rf_link.h"
#include "stc8h_delay.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA app_config_t config;
static STC8H_XDATA toy_remote_control_t control;
static STC8H_XDATA toy_remote_status_t rx_status;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
static STC8H_XDATA stc8h_u8 display_segments[4];
static app_radio_tx_result_t tx_result;
static stc8h_u16 tx_battery_centivolts;
static stc8h_u8 voltage_display_divider;
static stc8h_u8 show_rx_voltage;
static stc8h_u8 current_channel;
static stc8h_u8 radio_failures;
static STC8H_XDATA stc8h_u8 config_mode;
static STC8H_XDATA stc8h_u8 config_hold_ticks;
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
#define APP_CONFIG_ENTER_TICKS 60u
#define APP_CONFIG_ITEM_REDUCE 0u
#define APP_CONFIG_ITEM_MIDDLE 1u

static stc8h_u8 display_digit(stc8h_u8 value)
{
    static const STC8H_CODE stc8h_u8 table[10] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u,
        0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu
    };

    return table[value];
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
    (void)drv_tm1637_display_raw4(display_segments);
}

static void display_voltage(stc8h_u16 value)
{
    stc8h_u8 i;
    stc8h_u16 q;
    stc8h_u8 leading_blank;

    if (value > 9999u) {
        value = 9999u;
    }

    for (i = 4u; i != 0u; --i) {
        q = (stc8h_u16)(value / 10u);
        display_segments[i - 1u] = display_digit((stc8h_u8)(value - (stc8h_u16)(q * 10u)));
        value = q;
    }

    leading_blank = (display_segments[0] == display_digit(0u)) ? 1u : 0u;
    if (leading_blank != 0u) {
        display_segments[0] = APP_DISPLAY_BLANK;
    }
    if (show_rx_voltage == 0u) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    (void)drv_tm1637_display_raw4(display_segments);
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
    (void)drv_tm1637_display_raw4(display_segments);
}

static void enter_config_mode(void)
{
    config_mode = 1u;
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

void main(void)
{
    stc8h_spi_init();
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 1u, 2u);
    app_input_init(&control);
    if (app_config_load(&config) != STC8H_OK) {
        (void)app_config_save(&config);
    }
    control.tx_id = config.tx_id;
    current_channel = config.last_channel;
    drv_tm1637_init();
    rx_status.link_state = TOY_REMOTE_LINK_STATE_LOST;
    rx_status.voltage_int = 0u;
    rx_status.voltage_dec = 0u;
    rx_status.tx_id = 0u;
    tx_battery_centivolts = app_input_read_tx_battery_centivolts();

    if (app_radio_init_tx(current_channel) != STC8H_OK) {
        tx_result = APP_RADIO_TX_ERROR;
        while (1) {
        }
    }
    scan_channels();

    while (1) {
        if (config_mode != 0u) {
            handle_config_mode(app_input_update(&control));
        } else {
            app_input_update(&control);
            update_config_entry();
        }
        make_control_packet();
        tx_result = app_radio_send_packet_with_ack(packet);
        handle_ack_status();

        if (control.request_voltage != 0u) {
            update_voltage_display();
        } else {
            display_control();
        }
        if (tx_result == APP_RADIO_TX_DONE) {
            radio_failures = 0u;
        } else if (radio_failures < 10u) {
            ++radio_failures;
        } else {
            radio_failures = 0u;
            scan_channels();
        }
        stc8h_delay_ms(50u);
    }
}
