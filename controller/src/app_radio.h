#ifndef CONTROLLER_APP_RADIO_H
#define CONTROLLER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u
/* STATUS ACK is proto_rf_link's 9-byte header plus the 6-byte ToyRemote status payload.
 * Keep it shorter than the 32-byte control packet to reduce ACK airtime. */
#define APP_RADIO_STATUS_ACK_SIZE 15u
/* Match the legacy RF rate that was stable on this hardware: 1Mbps, 0dBm.
 * Code values follow drv_nrf24l01_rate_t and drv_nrf24l01_power_t. */
#define APP_RADIO_RATE_CODE 1u
#define APP_RADIO_POWER_CODE 3u
/* nRF24L01+ SETUP_RETR: ARD code 1 = 500us, ARC code 15 = 15 retries.
 * This is the Stc8hBase pair-diag stable set for 1Mbps + 15-byte ACK payload. */
#define APP_RADIO_RETRANSMIT_DELAY_CODE 1u
#define APP_RADIO_RETRANSMIT_COUNT_CODE 15u

#ifndef APP_RADIO_ENABLE_TX_DIAG_STATUS
#define APP_RADIO_ENABLE_TX_DIAG_STATUS 0
#endif

#ifndef APP_RADIO_ENABLE_STATS
#define APP_RADIO_ENABLE_STATS 0
#endif

typedef enum {
    APP_RADIO_TX_IDLE = 0,
    APP_RADIO_TX_DONE,
    APP_RADIO_TX_MAX_RETRY,
    APP_RADIO_TX_ACK_EMPTY,
    APP_RADIO_TX_ACK_PAYLOAD_OK,
    APP_RADIO_TX_ACK_BAD,
    APP_RADIO_TX_ERROR
} app_radio_tx_result_t;

#if APP_RADIO_ENABLE_STATS
typedef struct {
    stc8h_u16 tx_count;
    stc8h_u16 tx_ok;
    stc8h_u16 max_rt;
    stc8h_u16 ack_ok;
    stc8h_u16 ack_empty;
    stc8h_u16 ack_bad;
    stc8h_u8 last_status;
    stc8h_u8 fifo_status;
    stc8h_u8 observe_tx;
} app_radio_tx_stats_t;
#endif

stc8h_status_t app_radio_init_tx(stc8h_u8 channel);
void app_radio_set_channel(stc8h_u8 channel);
app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet);
void app_radio_recover_tx(void);

extern STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_STATUS_ACK_SIZE];
extern stc8h_u8 app_radio_ack_len;
#if APP_RADIO_ENABLE_TX_DIAG_STATUS
extern stc8h_u8 app_radio_last_status;
#endif
#if APP_RADIO_ENABLE_STATS
extern STC8H_XDATA app_radio_tx_stats_t app_radio_tx_stats;
#endif

#endif
