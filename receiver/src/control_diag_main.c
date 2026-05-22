#if APP_RECEIVER_CONTROL_DIAG_MAIN

#include "app_config.h"
#include "app_outputs.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_delay.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

#define CONTROL_DIAG_IDLE_LIMIT 60000u
#define CONTROL_DIAG_ERROR_BLINK_MS 100u

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA toy_remote_control_t control;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 status_packet[APP_RADIO_STATUS_ACK_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static stc8h_u16 idle_polls;
static stc8h_u16 ack_tx_id;
static stc8h_u8 ack_seq;
static stc8h_u8 brake_blink;
static stc8h_u8 brake_blink_divider;

static void control_diag_error_blink(void)
{
    while (1) {
        TOY_REMOTE_RX_LED_ON();
        stc8h_delay_ms(CONTROL_DIAG_ERROR_BLINK_MS);
        TOY_REMOTE_RX_LED_OFF();
        stc8h_delay_ms(CONTROL_DIAG_ERROR_BLINK_MS);
    }
}

static void control_diag_prepare_ack(stc8h_u16 tx_id, stc8h_u8 replace_pending)
{
    stc8h_u8 i;

    for (i = 0u; i < APP_RADIO_STATUS_ACK_SIZE; ++i) {
        status_packet[i] = 0u;
    }
    status_packet[0] = PROTO_RF_LINK_MAGIC;
    status_packet[1] = PROTO_RF_LINK_VERSION;
    status_packet[2] = PROTO_RF_LINK_PACKET_STATUS;
    status_packet[3] = ack_seq;
    status_packet[4] = packet[3];
    status_packet[5] = 0u;
    status_packet[6] = 2u;
    status_packet[7] = 1u;
    status_packet[8] = TOY_REMOTE_STATUS_PAYLOAD_SIZE;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_LINK_STATE] = TOY_REMOTE_LINK_STATE_CONNECTED;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] = 0u;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] = 0u;
    TOY_REMOTE_PUT_U16_LE(status_packet, PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_TX_ID_L, tx_id);
    ++ack_seq;

    if (replace_pending != 0u) {
        for (i = 0u; i < APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT; ++i) {
            (void)drv_nrf24l01_preload_ack_payload(0u, status_packet, APP_RADIO_STATUS_ACK_SIZE,
                                                   (i == 0u) ? 1u : 0u);
        }
    } else {
        (void)drv_nrf24l01_preload_ack_payload(0u, status_packet, APP_RADIO_STATUS_ACK_SIZE, 0u);
    }
}

static stc8h_status_t control_diag_unpack(void)
{
    if ((payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] > TOY_REMOTE_DIRECTION_REVERSE) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] > TOY_REMOTE_CONTROL_SPEED_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] > TOY_REMOTE_STEERING_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] > TOY_REMOTE_CONTROL_AUX_PWM_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] > 1u)) {
        return STC8H_ERROR;
    }

    control.direction = payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION];
    control.speed = payload[TOY_REMOTE_CONTROL_OFFSET_SPEED];
    control.brake = payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE];
    control.steering_angle = payload[TOY_REMOTE_CONTROL_OFFSET_STEERING];
    control.light = payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT];
    control.buzzer = payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER];
    control.aux_pwm = payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM];
    control.request_voltage = payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE];
    control.tx_id = TOY_REMOTE_GET_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L);
    return (control.tx_id == 0u) ? STC8H_ERROR : STC8H_OK;
}

static void control_diag_update_led(void)
{
    if (control.brake != 0u) {
        ++brake_blink_divider;
        if (brake_blink_divider >= 10u) {
            brake_blink_divider = 0u;
            brake_blink = (brake_blink == 0u) ? 1u : 0u;
        }
        if (brake_blink != 0u) {
            TOY_REMOTE_RX_LED_ON();
        } else {
            TOY_REMOTE_RX_LED_OFF();
        }
    } else if (control.speed >= 5u) {
        TOY_REMOTE_RX_LED_ON();
    } else {
        TOY_REMOTE_RX_LED_OFF();
    }
}

static void control_diag_handle_packet(void)
{
    stc8h_u8 replace_ack;

    if (proto_rf_link_poll_data_fixed(&link, packet, payload) != STC8H_OK) {
        return;
    }
    if (control_diag_unpack() != STC8H_OK) {
        return;
    }

    idle_polls = 0u;
    app_outputs_apply_control(&control);
    control_diag_update_led();

    replace_ack = (ack_tx_id == control.tx_id) ? 0u : 1u;
    ack_tx_id = control.tx_id;
    control_diag_prepare_ack(control.tx_id, replace_ack);
}

void main(void)
{
    drv_nrf24l01_init_pins();
    stc8h_spi_init();
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 2u, 1u);
    app_outputs_init();
    TOY_REMOTE_RX_LED_OFF();

    if (app_radio_init_rx(APP_CONFIG_DEFAULT_CHANNEL) != STC8H_OK) {
        app_outputs_apply_safe();
        control_diag_error_blink();
    }

    while (1) {
        if (app_radio_receive_packet(packet) == APP_RADIO_RX_PACKET) {
            control_diag_handle_packet();
        } else if (idle_polls < CONTROL_DIAG_IDLE_LIMIT) {
            ++idle_polls;
            if (idle_polls == CONTROL_DIAG_IDLE_LIMIT) {
                app_outputs_apply_safe();
                TOY_REMOTE_RX_LED_OFF();
            }
        }
    }
}

#else
typedef unsigned char control_diag_main_disabled_t;
#endif
