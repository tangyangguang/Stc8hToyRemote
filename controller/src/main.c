#include "app_radio.h"
#include "stc8h_spi.h"

static STC8H_XDATA stc8h_u8 packet[APP_RADIO_PACKET_SIZE];
static stc8h_u8 seq;
static app_radio_tx_result_t last_tx_result;

static void make_fixed_packet(void)
{
    stc8h_u8 i;

    for (i = 0u; i < APP_RADIO_PACKET_SIZE; ++i) {
        packet[i] = 0u;
    }

    packet[0] = 0xA5u;
    packet[1] = 0x01u;
    packet[2] = seq;
    packet[3] = (stc8h_u8)last_tx_result;
    ++seq;
}

void main(void)
{
    stc8h_spi_init();
    last_tx_result = APP_RADIO_TX_IDLE;

    if (app_radio_init_tx() != STC8H_OK) {
        last_tx_result = APP_RADIO_TX_ERROR;
    }

    while (1) {
        make_fixed_packet();
        if (last_tx_result != APP_RADIO_TX_ERROR) {
            last_tx_result = app_radio_send_packet(packet, APP_RADIO_PACKET_SIZE);
        }
    }
}
