#include "app_input.h"
#include "app_radio.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA toy_remote_control_t control;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[TOY_REMOTE_CONTROL_PAYLOAD_SIZE];
static app_radio_tx_result_t tx_result;

static stc8h_status_t make_control_packet(void)
{
    if (toy_remote_pack_control(payload, &control) != STC8H_OK) {
        return STC8H_ERROR;
    }

    return proto_rf_link_send_data(&link, packet, payload, TOY_REMOTE_CONTROL_PAYLOAD_SIZE);
}

void main(void)
{
    stc8h_spi_init();
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 1u, 2u);
    app_input_init(&control);

    if (app_radio_init_tx() != STC8H_OK) {
        tx_result = APP_RADIO_TX_ERROR;
        while (1) {
        }
    }

    while (1) {
        app_input_update(&control);
        if (make_control_packet() == STC8H_OK) {
            tx_result = app_radio_send_packet(packet, APP_RADIO_PACKET_SIZE);
        } else {
            tx_result = APP_RADIO_TX_ERROR;
        }
    }
}
