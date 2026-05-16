#include "app_radio.h"
#include "app_outputs.h"
#include "app_status.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA proto_rf_link_t link;
static STC8H_XDATA toy_remote_control_t control;
static STC8H_XDATA toy_remote_status_t status;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 status_packet[PROTO_RF_LINK_PACKET_SIZE];
static STC8H_XDATA stc8h_u8 payload[PROTO_RF_LINK_PAYLOAD_MAX];
static stc8h_u16 idle_polls;
static stc8h_u8 packet_count;
static stc8h_u8 radio_error;
static stc8h_u8 invalid_packet_count;
static stc8h_u8 link_lost;

#define APP_RECEIVER_IDLE_POLL_LIMIT 60000u

static void apply_safe_state(void)
{
    toy_remote_control_set_safe(&control);
    app_outputs_apply_safe();
    link_lost = 1u;
}

static void prepare_ack_status(void)
{
    stc8h_u8 i;

    app_status_update(&status, &control, link_lost);

    for (i = 0u; i < APP_RADIO_PACKET_SIZE; ++i) {
        status_packet[i] = 0u;
    }
    status_packet[0] = PROTO_RF_LINK_MAGIC;
    status_packet[1] = PROTO_RF_LINK_VERSION;
    status_packet[2] = PROTO_RF_LINK_PACKET_STATUS;
    status_packet[3] = link.seq_tx;
    status_packet[4] = link.seq_rx;
    status_packet[5] = 0u;
    status_packet[6] = 2u;
    status_packet[7] = 1u;
    status_packet[8] = TOY_REMOTE_STATUS_PAYLOAD_SIZE;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VERSION] = TOY_REMOTE_PROTOCOL_VERSION;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_LINK_STATE] = status.link_state;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_INT] = status.voltage_int;
    status_packet[PROTO_RF_LINK_HEADER_SIZE + TOY_REMOTE_STATUS_OFFSET_VOLTAGE_DEC] = status.voltage_dec;
    ++link.seq_tx;
    (void)app_radio_write_ack_packet(status_packet, APP_RADIO_PACKET_SIZE);
}

static void handle_packet(void)
{
    stc8h_u8 payload_len;

    payload_len = 0u;
    if (proto_rf_link_poll(&link, packet, 0, payload, &payload_len) == PROTO_RF_LINK_EVENT_DATA) {
        if (toy_remote_unpack_control(&control, payload, payload_len) == STC8H_OK) {
            idle_polls = 0u;
            link_lost = 0u;
            app_outputs_apply_control(&control);
            prepare_ack_status();
            return;
        }
    }

    ++invalid_packet_count;
}

static void handle_idle_poll(void)
{
    if (idle_polls < APP_RECEIVER_IDLE_POLL_LIMIT) {
        ++idle_polls;
        if (idle_polls == APP_RECEIVER_IDLE_POLL_LIMIT) {
            apply_safe_state();
        }
    }
}

void main(void)
{
    stc8h_spi_init();
    app_outputs_init();
    app_status_init(&status);
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 2u, 1u);
    apply_safe_state();

    if (app_radio_init_rx() != STC8H_OK) {
        radio_error = 1u;
    } else {
        prepare_ack_status();
    }

    while (1) {
        if (radio_error == 0u) {
            if (app_radio_receive_packet(packet, APP_RADIO_PACKET_SIZE) == APP_RADIO_RX_PACKET) {
                handle_packet();
                ++packet_count;
            } else {
                handle_idle_poll();
            }
        } else {
            apply_safe_state();
        }
    }
}
