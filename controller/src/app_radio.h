#ifndef CONTROLLER_APP_RADIO_H
#define CONTROLLER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u
/* STATUS ACK is proto_rf_link's 9-byte header plus the 6-byte ToyRemote status payload.
 * Keep it shorter than the 32-byte control packet to reduce 250kbps ACK airtime. */
#define APP_RADIO_STATUS_ACK_SIZE 15u
/* nRF24L01+ SETUP_RETR.ARD code 5 = 1500us. This is retained as margin for
 * 250kbps ACK payloads while ARC=10 keeps send latency bounded. */
#define APP_RADIO_RETRANSMIT_DELAY_CODE 5u
#define APP_RADIO_RETRANSMIT_COUNT_CODE 10u

#ifndef APP_RADIO_ENABLE_TX_DIAG_STATUS
#define APP_RADIO_ENABLE_TX_DIAG_STATUS 0
#endif

typedef enum {
    APP_RADIO_TX_IDLE = 0,
    APP_RADIO_TX_DONE,
    APP_RADIO_TX_MAX_RETRY,
    APP_RADIO_TX_ERROR
} app_radio_tx_result_t;

stc8h_status_t app_radio_init_tx(stc8h_u8 channel);
void app_radio_set_channel(stc8h_u8 channel);
app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet);

extern STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_PACKET_SIZE];
extern stc8h_u8 app_radio_ack_len;
#if APP_RADIO_ENABLE_TX_DIAG_STATUS
extern stc8h_u8 app_radio_last_status;
#endif

#endif
