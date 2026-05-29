#if APP_RECEIVER_CONTROL_GPIO_DIAG_MAIN

#include "app_config.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

#define CONTROL_GPIO_DIAG_IDLE_LIMIT 60000u

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 status_packet[APP_RADIO_STATUS_ACK_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static stc8h_u16 idle_polls;
static stc8h_u16 ack_tx_id;
static stc8h_u8 ack_seq;

static void control_gpio_diag_stop(void)
{
    TOY_REMOTE_RX_MOTOR_STOP();
    TOY_REMOTE_RX_LED_OFF();
}

static void control_gpio_diag_forward(void)
{
    TOY_REMOTE_RX_MOTOR_IN1_BIT = 1;
    TOY_REMOTE_RX_MOTOR_IN2_BIT = 0;
    TOY_REMOTE_RX_LED_ON();
}

static void control_gpio_diag_reverse(void)
{
    TOY_REMOTE_RX_MOTOR_IN1_BIT = 0;
    TOY_REMOTE_RX_MOTOR_IN2_BIT = 1;
    TOY_REMOTE_RX_LED_ON();
}

static void control_gpio_diag_prepare_ack(stc8h_u16 tx_id, stc8h_u8 replace_pending)
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
    TOY_REMOTE_PUT_U16_LE(status_packet, PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_TX_ID_L, tx_id);
    ++ack_seq;

    if (replace_pending != 0u) {
        for (i = 0u; i < APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT; ++i) {
            (void)drv_nrf24l01_preload_ack_payload(0u, status_packet, APP_RADIO_STATUS_ACK_SIZE,
                                                   (i == 0u) ? APP_RADIO_ACK_PAYLOAD_REPLACE_ON_RECOVER :
                                                               APP_RADIO_ACK_PAYLOAD_REPLACE_AFTER_RX);
        }
    } else {
        (void)drv_nrf24l01_preload_ack_payload(0u, status_packet, APP_RADIO_STATUS_ACK_SIZE,
                                               APP_RADIO_ACK_PAYLOAD_REPLACE_AFTER_RX);
    }
}

static stc8h_status_t control_gpio_diag_valid_payload(void)
{
    if ((payload[TOY_REMOTE_CONTROL_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] > TOY_REMOTE_DIRECTION_REVERSE) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] > TOY_REMOTE_CONTROL_SPEED_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_STEERING] > TOY_REMOTE_STEERING_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_LIGHT] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_BUZZER] > 1u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] > TOY_REMOTE_CONTROL_AUX_PWM_MAX) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] > 1u) ||
        (TOY_REMOTE_GET_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L) == 0u)) {
        return STC8H_ERROR;
    }
    return STC8H_OK;
}

static void control_gpio_diag_handle_packet(void)
{
    stc8h_u16 tx_id;
    stc8h_u8 replace_ack;

    if (proto_rf_link_poll_data_fixed_xdata(&link, packet, payload) != STC8H_OK) {
        return;
    }
    if (control_gpio_diag_valid_payload() != STC8H_OK) {
        return;
    }

    idle_polls = 0u;
    tx_id = TOY_REMOTE_GET_U16_LE(payload, TOY_REMOTE_CONTROL_OFFSET_TX_ID_L);
    replace_ack = (ack_tx_id == tx_id) ? 0u : 1u;
    ack_tx_id = tx_id;
    control_gpio_diag_prepare_ack(tx_id, replace_ack);

    if ((payload[TOY_REMOTE_CONTROL_OFFSET_BRAKE] != 0u) ||
        (payload[TOY_REMOTE_CONTROL_OFFSET_SPEED] < 5u)) {
        control_gpio_diag_stop();
    } else if (payload[TOY_REMOTE_CONTROL_OFFSET_DIRECTION] == TOY_REMOTE_DIRECTION_REVERSE) {
        control_gpio_diag_reverse();
    } else {
        control_gpio_diag_forward();
    }
}

void main(void)
{
    P3M0 |= (stc8h_u8)(TOY_REMOTE_RX_MOTOR_IN1_MASK |
                       TOY_REMOTE_RX_MOTOR_IN2_MASK |
                       TOY_REMOTE_RX_LED_MASK);
    P3M1 &= (stc8h_u8)~(TOY_REMOTE_RX_MOTOR_IN1_MASK |
                        TOY_REMOTE_RX_MOTOR_IN2_MASK |
                        TOY_REMOTE_RX_LED_MASK);
    control_gpio_diag_stop();

    drv_nrf24l01_init_pins();
    stc8h_spi_init();
    proto_rf_link_init_xdata(&link);
    proto_rf_link_set_ids_xdata(&link, 2u, 1u);

    if (app_radio_init_rx(APP_CONFIG_DEFAULT_CHANNEL) != STC8H_OK) {
        while (1) {
            TOY_REMOTE_RX_LED_ON();
        }
    }

    while (1) {
        if (app_radio_receive_packet(packet) == APP_RADIO_RX_PACKET) {
            control_gpio_diag_handle_packet();
        } else if (idle_polls < CONTROL_GPIO_DIAG_IDLE_LIMIT) {
            ++idle_polls;
            if (idle_polls == CONTROL_GPIO_DIAG_IDLE_LIMIT) {
                control_gpio_diag_stop();
            }
        }
    }
}

#else
typedef unsigned char control_gpio_diag_main_disabled_t;
#endif
