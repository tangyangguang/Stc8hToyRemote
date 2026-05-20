#ifndef RECEIVER_APP_RADIO_H
#define RECEIVER_APP_RADIO_H

#include "stc8h_config.h"

#define APP_RADIO_PACKET_SIZE 32u
#define APP_RADIO_STATUS_ACK_SIZE 15u
#define APP_RADIO_RATE_CODE 1u
#define APP_RADIO_POWER_CODE 3u
/* nRF24 ACK payload uses the TX FIFO on PRX. Preload all 3 slots so
 * retransmitted packets do not drain the FIFO into ACK_EMPTY. */
#define APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT 3u

typedef enum {
    APP_RADIO_RX_NONE = 0,
    APP_RADIO_RX_PACKET,
    APP_RADIO_RX_ERROR
} app_radio_rx_result_t;

stc8h_status_t app_radio_init_rx(stc8h_u8 channel);
void app_radio_set_channel(stc8h_u8 channel);
app_radio_rx_result_t app_radio_receive_packet(stc8h_u8 *packet);

#endif
