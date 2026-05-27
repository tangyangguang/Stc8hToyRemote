#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_ADDR_LEN 5u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};

stc8h_status_t app_radio_init_rx(stc8h_u8 channel)
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
    if (drv_nrf24l01_config_pipe0_fixed(app_radio_addr) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_enable_ack_payload(DRV_NRF24L01_PIPE0) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_set_auto_ack(DRV_NRF24L01_PIPE0);
    if (drv_nrf24l01_set_auto_retransmit(APP_RADIO_RETRANSMIT_DELAY_CODE, APP_RADIO_RETRANSMIT_COUNT_CODE) != STC8H_OK) {
        return STC8H_ERROR;
    }
    if (drv_nrf24l01_set_rate_power((drv_nrf24l01_rate_t)APP_RADIO_RATE_CODE,
                                    (drv_nrf24l01_power_t)APP_RADIO_POWER_CODE) != STC8H_OK) {
        return STC8H_ERROR;
    }

    drv_nrf24l01_enter_rx();
    return STC8H_OK;
}

void app_radio_set_channel(stc8h_u8 channel)
{
    (void)drv_nrf24l01_set_channel(channel);
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(0x70u);
    drv_nrf24l01_enter_rx();
}

app_radio_rx_result_t app_radio_receive_packet(stc8h_u8 *packet)
{
    stc8h_u8 status;

    status = drv_nrf24l01_read_status();
    if ((status & DRV_NRF24L01_STATUS_RX_READY) == 0u) {
        return APP_RADIO_RX_NONE;
    }

#if DRV_NRF24L01_ENABLE_FIXED_PAYLOAD_API
    (void)drv_nrf24l01_read_payload_fixed(packet);
#else
    (void)drv_nrf24l01_read_payload(packet, APP_RADIO_PACKET_SIZE);
#endif
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(status);
    return APP_RADIO_RX_PACKET;
}
