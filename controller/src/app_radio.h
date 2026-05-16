#ifndef CONTROLLER_APP_RADIO_H
#define CONTROLLER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u

typedef enum {
    APP_RADIO_TX_IDLE = 0,
    APP_RADIO_TX_DONE,
    APP_RADIO_TX_MAX_RETRY,
    APP_RADIO_TX_ERROR
} app_radio_tx_result_t;

stc8h_status_t app_radio_init_tx(stc8h_u8 channel);
stc8h_status_t app_radio_set_channel(stc8h_u8 channel);
app_radio_tx_result_t app_radio_send_packet_with_ack(const stc8h_u8 *packet, stc8h_u8 len);

extern STC8H_XDATA stc8h_u8 app_radio_ack_packet[APP_RADIO_PACKET_SIZE];
extern stc8h_u8 app_radio_ack_len;

#endif
