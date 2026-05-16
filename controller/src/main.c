#include "app_config.h"
#include "app_input.h"
#include "app_radio.h"
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

#define APP_DISPLAY_BLANK 0x00u
#define APP_DISPLAY_DASH 0x40u
#define APP_DISPLAY_A 0x77u
#define APP_DISPLAY_UP 0x23u
#define APP_DISPLAY_DOWN 0x1Cu
#define APP_DISPLAY_COLON 0x80u

static stc8h_u8 display_digit(stc8h_u8 value)
{
    if (value > 9u) {
        return APP_DISPLAY_BLANK;
    }
    return drv_tm1637_encode_digit(value);
}

static stc8h_u8 display_speed_tens(stc8h_u8 speed)
{
    return (speed >= 100u) ? APP_DISPLAY_A : display_digit((stc8h_u8)(speed / 10u));
}

static void display_init(void)
{
    drv_tm1637_init();
    drv_tm1637_set_brightness(1u);
    drv_tm1637_set_display(1u);
    (void)drv_tm1637_clear();
}

static void display_control(stc8h_u8 tx_ok)
{
    if (control.brake != 0u) {
        display_segments[0] = APP_DISPLAY_DASH;
        display_segments[1] = APP_DISPLAY_BLANK;
        display_segments[2] = (control.speed == 0u) ? APP_DISPLAY_DASH : display_speed_tens(control.speed);
        display_segments[3] = (control.speed == 0u) ? APP_DISPLAY_DASH : display_digit((stc8h_u8)(control.speed % 10u));
    } else {
        display_segments[0] = (control.direction == TOY_REMOTE_DIRECTION_REVERSE) ? APP_DISPLAY_DOWN : APP_DISPLAY_UP;
        display_segments[1] = APP_DISPLAY_BLANK;
        display_segments[2] = display_speed_tens(control.speed);
        display_segments[3] = display_digit((stc8h_u8)(control.speed % 10u));
    }

    if (tx_ok == 0u) {
        display_segments[1] |= APP_DISPLAY_COLON;
    }
    (void)drv_tm1637_display_raw(display_segments, 4u);
}

static void display_voltage(stc8h_u16 value, stc8h_u8 show_rx)
{
    if (value > 9999u) {
        value = 9999u;
    }

    display_segments[0] = (value >= 1000u) ? display_digit((stc8h_u8)(value / 1000u)) : APP_DISPLAY_BLANK;
    value %= 1000u;
    display_segments[1] = (stc8h_u8)(display_digit((stc8h_u8)(value / 100u)) | ((show_rx == 0u) ? APP_DISPLAY_COLON : 0u));
    value %= 100u;
    display_segments[2] = display_digit((stc8h_u8)(value / 10u));
    display_segments[3] = display_digit((stc8h_u8)(value % 10u));
    (void)drv_tm1637_display_raw(display_segments, 4u);
}

static stc8h_status_t make_control_packet(void)
{
    control.tx_id = config.tx_id;
    payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] = control.direction;
    payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] = control.speed;
    payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] = control.brake;
    payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] = control.steering_angle;
    payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] = control.light;
    payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] = control.buzzer;
    payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] = control.aux_pwm;
    payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] = control.request_voltage;
    TOY_REMOTE_PUT_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L, control.tx_id);
    return proto_rf_link_send_data(&link, packet, payload, TOY_REMOTE_CONTROL_PAYLOAD_SIZE);
}

static void handle_ack_status(stc8h_u8 ack_len)
{
    const stc8h_u8 *ack;

    if (ack_len != APP_RADIO_PACKET_SIZE) {
        return;
    }

    ack = app_radio_ack_packet;
    if ((ack[0] != PROTO_RF_LINK_MAGIC) ||
        (ack[1] != PROTO_RF_LINK_VERSION) ||
        (ack[2] != PROTO_RF_LINK_PACKET_STATUS) ||
        (ack[6] != 2u) ||
        (ack[7] != 1u) ||
        (ack[8] != TOY_REMOTE_STATUS_PAYLOAD_SIZE) ||
        (ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_TX_ID_L] != (stc8h_u8)config.tx_id) ||
        (ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_TX_ID_H] != (stc8h_u8)(config.tx_id >> 8))) {
        return;
    }
    if ((ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_LINK_STATE] > TOY_REMOTE_LINK_STATE_LOST) ||
        (ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] > TOY_REMOTE_VOLTAGE_INT_MAX) ||
        (ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] > TOY_REMOTE_VOLTAGE_DEC_MAX)) {
        return;
    }

    link.seq_rx = ack[3];
    link.ack_pending = 0u;
    link.state = PROTO_RF_LINK_STATE_CONNECTED;
    rx_status.link_state = ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_LINK_STATE];
    rx_status.voltage_int = ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT];
    rx_status.voltage_dec = ack[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC];
    rx_status.tx_id = config.tx_id;
}

static stc8h_u8 probe_current_channel(void)
{
    stc8h_u8 i;

    for (i = 0u; i < 2u; ++i) {
        if (make_control_packet() == STC8H_OK) {
            tx_result = app_radio_send_packet_with_ack(packet, APP_RADIO_PACKET_SIZE);
            handle_ack_status(app_radio_ack_len);
            if (rx_status.tx_id == config.tx_id) {
                return 1u;
            }
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
        if (app_radio_set_channel(channel) != STC8H_OK) {
            continue;
        }
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
    (void)app_radio_set_channel(current_channel);
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
        display_voltage((stc8h_u16)((stc8h_u16)rx_status.voltage_int * 100u + rx_status.voltage_dec), 1u);
    } else {
        display_voltage(tx_battery_centivolts, 0u);
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
    display_init();
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
        app_input_update(&control);
        if (make_control_packet() == STC8H_OK) {
            tx_result = app_radio_send_packet_with_ack(packet, APP_RADIO_PACKET_SIZE);
            handle_ack_status(app_radio_ack_len);
        } else {
            tx_result = APP_RADIO_TX_ERROR;
        }

        if (control.request_voltage != 0u) {
            update_voltage_display();
        } else {
            display_control((tx_result == APP_RADIO_TX_DONE) ? 1u : 0u);
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
