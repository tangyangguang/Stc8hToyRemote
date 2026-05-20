#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_ADDR_LEN 5u
#define APP_RADIO_TX_WAIT_LIMIT 6000u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};
STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_PACKET_SIZE];
stc8h_u8 app_radio_ack_len;

stc8h_status_t app_radio_init_tx(stc8h_u8 channel)
{
    drv_nrf24l01_init_pins();
    drv_nrf24l01_power_down();
    drv_nrf24l01_flush_tx();
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(0x70u);

    if (drv_nrf24l01_set_channel(channel) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_config_pipe0_fixed(app_radio_addr) != STC8H_OK) {
        return STC8H_ERROR;
    }
    (void)drv_nrf24l01_enable_ack_payload(DRV_NRF24L01_PIPE0);

    drv_nrf24l01_set_auto_ack(DRV_NRF24L01_PIPE0);
    if (drv_nrf24l01_set_auto_retransmit(3u, 3u) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rate_power(DRV_NRF24L01_RATE_250KBPS, DRV_NRF24L01_POWER_0DBM) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_enter_tx();
    return STC8H_OK;
}

void app_radio_set_channel(stc8h_u8 channel)
{
    (void)drv_nrf24l01_set_channel(channel);
    drv_nrf24l01_enter_tx();
}

app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet)
{
    stc8h_u8 status;
    stc8h_u16 wait;

    app_radio_ack_len = 0u;

    drv_nrf24l01_flush_tx();
    (void)drv_nrf24l01_write_payload(packet, APP_RADIO_PACKET_SIZE);
    drv_nrf24l01_pulse_ce();

    for (wait = 0u; wait < APP_RADIO_TX_WAIT_LIMIT; ++wait) {
        status = drv_nrf24l01_read_status();
        if ((status & DRV_NRF24L01_STATUS_TX_DONE) != 0u) {
            if ((status & DRV_NRF24L01_STATUS_RX_READY) != 0u) {
                (void)drv_nrf24l01_read_payload(app_radio_ack_packet, APP_RADIO_PACKET_SIZE);
                app_radio_ack_len = APP_RADIO_PACKET_SIZE;
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
