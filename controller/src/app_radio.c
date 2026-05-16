#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_ADDR_LEN 5u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};
static STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_PACKET_SIZE];
static stc8h_u8 app_radio_ack_len;

stc8h_status_t app_radio_init_tx(stc8h_u8 channel)
{
    drv_nrf24l01_init_pins();
    drv_nrf24l01_power_down();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(0x70u);

    if (drv_nrf24l01_check_present() != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_channel(channel) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_address_width(APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_tx_address(app_radio_addr, APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rx_address(0u, app_radio_addr, APP_RADIO_ADDR_LEN) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_payload_size(0u, APP_RADIO_PACKET_SIZE) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_enable_ack_payload(DRV_NRF24L01_PIPE0) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_set_auto_ack(DRV_NRF24L01_PIPE0);
    if (drv_nrf24l01_set_auto_retransmit(3u, 10u) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rate_power(DRV_NRF24L01_RATE_1MBPS, DRV_NRF24L01_POWER_0DBM) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_enter_tx();
    return STC8H_OK;
}

stc8h_status_t app_radio_set_channel(stc8h_u8 channel)
{
    if (drv_nrf24l01_set_channel(channel) != STC8H_OK) {
        return STC8H_ERROR;
    }
    drv_nrf24l01_enter_tx();
    return STC8H_OK;
}

app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet, stc8h_u8 len)
{
    stc8h_u8 status;
    stc8h_u8 ack_width;
    stc8h_u16 wait;

    if ((packet == 0) || (len != APP_RADIO_PACKET_SIZE)) {
        return APP_RADIO_TX_ERROR;
    }
    app_radio_ack_len = 0u;

    drv_nrf24l01_flush_tx();
    (void)drv_nrf24l01_write_payload(packet, len);
    drv_nrf24l01_pulse_ce();

    for (wait = 0u; wait < 60000u; ++wait) {
        status = drv_nrf24l01_read_status();
        if ((status & DRV_NRF24L01_STATUS_TX_DONE) != 0u) {
            if ((status & DRV_NRF24L01_STATUS_RX_READY) != 0u) {
                ack_width = drv_nrf24l01_read_dynamic_payload_size();
                if ((ack_width > 0u) && (ack_width <= APP_RADIO_PACKET_SIZE)) {
                    (void)drv_nrf24l01_read_payload(app_radio_ack_packet, ack_width);
                    app_radio_ack_len = ack_width;
                } else {
                    drv_nrf24l01_flush_rx();
                }
            }
            drv_nrf24l01_clear_irq(status);
            return APP_RADIO_TX_DONE;
        }
        if ((status & DRV_NRF24L01_STATUS_MAX_RETRY) != 0u) {
            drv_nrf24l01_flush_tx();
            drv_nrf24l01_clear_irq(status);
            return APP_RADIO_TX_MAX_RETRY;
        }
    }

    status = drv_nrf24l01_read_status();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_clear_irq(status);
    return APP_RADIO_TX_ERROR;
}

const stc8h_u8 *app_radio_get_ack_packet(void)
{
    return app_radio_ack_packet;
}

stc8h_u8 app_radio_get_ack_len(void)
{
    return app_radio_ack_len;
}
