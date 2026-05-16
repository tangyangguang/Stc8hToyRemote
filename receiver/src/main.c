#include "app_radio.h"
#include "app_outputs.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static proto_rf_link_t link;
static toy_remote_control_t control;
static STC8H_XDATA stc8h_u8 packet[PROTO_RF_LINK_PACKET_SIZE];
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

static void handle_packet(void)
{
    stc8h_u8 payload_len;

    payload_len = 0u;
    if (proto_rf_link_poll(&link, packet, 0, payload, &payload_len) == PROTO_RF_LINK_EVENT_DATA) {
        if (toy_remote_unpack_control(&control, payload, payload_len) == STC8H_OK) {
            idle_polls = 0u;
            link_lost = 0u;
            app_outputs_apply_control(&control);
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
    proto_rf_link_init(&link);
    proto_rf_link_set_ids(&link, 2u, 1u);
    apply_safe_state();

    if (app_radio_init_rx() != STC8H_OK) {
        radio_error = 1u;
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
