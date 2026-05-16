#ifndef RECEIVER_APP_RADIO_H
#define RECEIVER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u

typedef enum {
    APP_RADIO_RX_NONE = 0,
    APP_RADIO_RX_PACKET,
    APP_RADIO_RX_ERROR
} app_radio_rx_result_t;

stc8h_status_t app_radio_init_rx(void);
app_radio_rx_result_t app_radio_receive_packet(stc8h_u8 *packet, stc8h_u8 len);

#endif
