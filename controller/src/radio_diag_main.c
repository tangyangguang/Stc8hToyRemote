#if APP_RADIO_LINK_DIAG_MAIN

#include "app_config.h"
#include "app_display.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_tm1637.h"
#include "proto_rf_link.h"
#include "stc8h_delay.h"
#include "stc8h_sfr.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

#define APP_RADIO_DIAG_CHANNEL APP_DEFAULT_RF_CHANNEL
#define APP_RADIO_DIAG_TX_ID APP_TX_ID
#define APP_RADIO_DIAG_T 0x78u

static STC8H_XDATA stc8h_u8 packet[APP_RADIO_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 segments[4];
static stc8h_u8 seq_tx;

static void display_raw4(void)
{
    (void)drv_tm1637_display_raw4(segments);
}

static void display_prefixed(stc8h_u8 prefix, stc8h_u8 value)
{
    app_display_prefixed_channel_segments(prefix, value, segments);
    display_raw4();
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
}

static void make_diag_packet(void)
{
    stc8h_u8 i;

    for (i = 0u; i < APP_RADIO_PACKET_SIZE; ++i) {
        packet[i] = 0u;
    }
    packet[0] = PROTO_RF_LINK_MAGIC;
    packet[1] = PROTO_RF_LINK_VERSION;
    packet[2] = PROTO_RF_LINK_PACKET_DATA;
    packet[3] = seq_tx;
    packet[4] = 0u;
    packet[5] = PROTO_RF_LINK_FLAG_ACK_REQUIRED;
    packet[6] = 1u;
    packet[7] = 2u;
    packet[8] = TOY_REMOTE_CONTROL_PAYLOAD_SIZE;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_DIRECTION] = TOY_REMOTE_DIRECTION_FORWARD;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_SPEED] = 0u;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_BRAKE] = 1u;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_STEERING] = TOY_REMOTE_STEERING_CENTER;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_LIGHT] = 0u;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_BUZZER] = 0u;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_AUX_PWM] = 0u;
    packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_REQUEST_VOLTAGE] = 0u;
    TOY_REMOTE_PUT_U16_LE(packet, PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_CONTROL_OFFSET_TX_ID_L, APP_RADIO_DIAG_TX_ID);
    ++seq_tx;
}

static stc8h_u8 ack_protocol_status(void)
{
    const stc8h_u8 *ack;
    const stc8h_u8 *body;

    if (app_radio_ack_len == 0u) {
        return 0u;
    }
    if (app_radio_ack_len != APP_RADIO_PACKET_SIZE) {
        return 1u;
    }

    ack = app_radio_ack_packet;
    body = &ack[PROTO_RF_LINK_HEADER_SIZE];
    if ((ack[0] != PROTO_RF_LINK_MAGIC) ||
        (ack[1] != PROTO_RF_LINK_VERSION) ||
        (ack[2] != PROTO_RF_LINK_PACKET_STATUS) ||
        (ack[6] != 2u) ||
        (ack[7] != 1u) ||
        (ack[8] != TOY_REMOTE_STATUS_PAYLOAD_SIZE)) {
        return 2u;
    }
    if ((body[TOY_REMOTE_STATUS_OFFSET_TX_ID_L] != (stc8h_u8)APP_RADIO_DIAG_TX_ID) ||
        (body[TOY_REMOTE_STATUS_OFFSET_TX_ID_H] != (stc8h_u8)(APP_RADIO_DIAG_TX_ID >> 8))) {
        return 3u;
    }
    if ((body[TOY_REMOTE_STATUS_OFFSET_VERSION] != TOY_REMOTE_PROTOCOL_VERSION) ||
        (body[TOY_REMOTE_STATUS_OFFSET_LINK_STATE] > TOY_REMOTE_LINK_STATE_LOST) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] > TOY_REMOTE_VOLTAGE_INT_MAX) ||
        (body[TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] > TOY_REMOTE_VOLTAGE_DEC_MAX)) {
        return 4u;
    }
    return 5u;
}

static void display_tx_result(app_radio_tx_result_t result)
{
    stc8h_u8 ack_status;

    if (result == APP_RADIO_TX_MAX_RETRY) {
        display_prefixed(APP_DISPLAY_L, APP_RADIO_DIAG_CHANNEL);
        return;
    }
    if (result == APP_RADIO_TX_ERROR) {
        display_prefixed(APP_DISPLAY_E, APP_RADIO_DIAG_CHANNEL);
        return;
    }

    ack_status = ack_protocol_status();
    if (ack_status == 0u) {
        display_prefixed(APP_RADIO_DIAG_T, APP_RADIO_DIAG_CHANNEL);
    } else if (ack_status == 1u) {
        display_prefixed(APP_DISPLAY_P, app_radio_ack_len);
    } else if (ack_status == 5u) {
        display_prefixed(APP_DISPLAY_F, APP_RADIO_DIAG_CHANNEL);
    } else {
        display_prefixed(APP_DISPLAY_A, ack_status);
    }
}

void main(void)
{
    app_radio_tx_result_t result;

    stc8h_spi_init();
    display_init();
    display_prefixed(APP_DISPLAY_C, APP_RADIO_DIAG_CHANNEL);
    stc8h_delay_ms(150u);

    if (app_radio_init_tx(APP_RADIO_DIAG_CHANNEL) != STC8H_OK) {
        display_prefixed(APP_DISPLAY_E, 1u);
        while (1) {
        }
    }

    while (1) {
        make_diag_packet();
        result = app_radio_send_packet_with_ack(packet);
        display_tx_result(result);
        stc8h_delay_ms(250u);
    }
}

#else
typedef unsigned char radio_diag_main_disabled_t;
#endif
