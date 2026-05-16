#include "app_radio.h"
#include "drv_nrf24l01.h"
#include "proto_rf_link.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

static STC8H_XDATA stc8h_u8 packet[APP_RADIO_PACKET_SIZE];
static stc8h_u8 last_seq;
static stc8h_u8 packet_count;
static stc8h_u8 radio_error;

static void handle_packet(void)
{
    if ((packet[0] == 0xA5u) && (packet[1] == 0x01u)) {
        last_seq = packet[2];
        ++packet_count;
    }
}

void main(void)
{
    stc8h_spi_init();

    if (app_radio_init_rx() != STC8H_OK) {
        radio_error = 1u;
    }

    while (1) {
        if (radio_error == 0u) {
            if (app_radio_receive_packet(packet, APP_RADIO_PACKET_SIZE) == APP_RADIO_RX_PACKET) {
                handle_packet();
            }
        }
    }
}
