#include "app_radio.h"
#include "drv_nrf24l01.h"

#define APP_RADIO_ADDR_LEN 5u
#define APP_RADIO_TX_WAIT_LIMIT 6000u

static const stc8h_u8 app_radio_addr[APP_RADIO_ADDR_LEN] = {'T', 'O', 'Y', 'R', '1'};
STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_STATUS_ACK_SIZE];
stc8h_u8 app_radio_ack_len;
#if APP_RADIO_ENABLE_TX_DIAG_STATUS
stc8h_u8 app_radio_last_status;
#define app_radio_set_last_status(status) do { app_radio_last_status = (status); } while (0)
#else
#define app_radio_set_last_status(status) do { (void)(status); } while (0)
#endif
#if APP_RADIO_ENABLE_STATS
STC8H_XDATA app_radio_tx_stats_t app_radio_tx_stats;
#define app_radio_stats_inc(field) do { ++app_radio_tx_stats.field; } while (0)
static void app_radio_stats_sample(stc8h_u8 status)
{
    app_radio_tx_stats.last_status = status;
#if DRV_NRF24L01_ENABLE_READ_FIFO_STATUS
    app_radio_tx_stats.fifo_status = drv_nrf24l01_read_fifo_status();
#endif
#if DRV_NRF24L01_ENABLE_READ_OBSERVE_TX
    app_radio_tx_stats.observe_tx = drv_nrf24l01_read_observe_tx();
#endif
}
#else
#define app_radio_stats_inc(field) do { } while (0)
#define app_radio_stats_sample(status) do { (void)(status); } while (0)
#endif

#if APP_RADIO_ENABLE_STATS
static void app_radio_record_result(app_radio_tx_result_t result)
{
    if ((result == APP_RADIO_TX_DONE) ||
        (result == APP_RADIO_TX_ACK_PAYLOAD_OK) ||
        (result == APP_RADIO_TX_ACK_EMPTY) ||
        (result == APP_RADIO_TX_ACK_BAD)) {
        app_radio_stats_inc(tx_ok);
    }
    if (result == APP_RADIO_TX_ACK_PAYLOAD_OK) {
        app_radio_stats_inc(ack_ok);
    } else if (result == APP_RADIO_TX_ACK_EMPTY) {
        app_radio_stats_inc(ack_empty);
    } else if (result == APP_RADIO_TX_ACK_BAD) {
        app_radio_stats_inc(ack_bad);
    } else if (result == APP_RADIO_TX_MAX_RETRY) {
        app_radio_stats_inc(max_rt);
    }
}
#else
#define app_radio_record_result(result) do { (void)(result); } while (0)
#endif

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

    drv_nrf24l01_enter_tx();
    return STC8H_OK;
}

void app_radio_set_channel(stc8h_u8 channel)
{
    (void)drv_nrf24l01_set_channel(channel);
    app_radio_recover_tx();
}

void app_radio_recover_tx(void)
{
    drv_nrf24l01_flush_rx();
    drv_nrf24l01_clear_irq(DRV_NRF24L01_IRQ_MASK);
    drv_nrf24l01_enter_tx();
}

app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet)
{
    stc8h_u8 status;
    stc8h_u16 wait;
    app_radio_tx_result_t result;
    stc8h_u8 width;

    app_radio_ack_len = 0u;
    app_radio_stats_inc(tx_count);

    drv_nrf24l01_flush_tx();
    (void)drv_nrf24l01_write_payload(packet, APP_RADIO_PACKET_SIZE);
    drv_nrf24l01_pulse_ce();

    for (wait = 0u; wait < APP_RADIO_TX_WAIT_LIMIT; ++wait) {
        status = drv_nrf24l01_read_status();
        app_radio_set_last_status(status);
        if ((status & (DRV_NRF24L01_STATUS_TX_DONE | DRV_NRF24L01_STATUS_MAX_RETRY)) != 0u) {
            if ((status & DRV_NRF24L01_STATUS_MAX_RETRY) != 0u) {
                drv_nrf24l01_flush_tx();
                drv_nrf24l01_clear_irq(DRV_NRF24L01_STATUS_MAX_RETRY);
                result = APP_RADIO_TX_MAX_RETRY;
            } else if ((status & DRV_NRF24L01_STATUS_RX_READY) != 0u) {
                width = drv_nrf24l01_read_dynamic_payload_size();
                if (width == APP_RADIO_STATUS_ACK_SIZE) {
                    (void)drv_nrf24l01_read_payload(app_radio_ack_packet, width);
                    app_radio_ack_len = width;
                    result = APP_RADIO_TX_ACK_PAYLOAD_OK;
                } else {
                    drv_nrf24l01_flush_rx();
                    result = (width == 0u) ? APP_RADIO_TX_ACK_EMPTY : APP_RADIO_TX_ACK_BAD;
                }
                drv_nrf24l01_clear_irq((stc8h_u8)(status | DRV_NRF24L01_STATUS_RX_READY));
            } else {
                drv_nrf24l01_clear_irq(status);
                result = APP_RADIO_TX_ACK_EMPTY;
            }
            app_radio_stats_sample(status);
            app_radio_record_result(result);
            if (result == APP_RADIO_TX_MAX_RETRY) {
                app_radio_recover_tx();
            }
            return result;
        }
    }

    status = drv_nrf24l01_read_status();
    app_radio_set_last_status(status);
    app_radio_stats_sample(status);
    app_radio_recover_tx();
    return APP_RADIO_TX_ERROR;
}
