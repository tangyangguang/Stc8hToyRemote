#include "app_radio.h"

#include <assert.h>

#ifndef APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT
#error "receiver must document how many ACK payloads are preloaded."
#endif

#ifndef APP_RADIO_STATUS_ACK_SIZE
#error "receiver must document status ACK payload length."
#endif

#ifndef APP_RADIO_RATE_CODE
#error "receiver must document nRF24 RF data rate."
#endif

#ifndef APP_RADIO_POWER_CODE
#error "receiver must document nRF24 RF output power."
#endif

#ifndef APP_RADIO_RETRANSMIT_DELAY_CODE
#error "receiver must document the nRF24 SETUP_RETR.ARD value."
#endif

#ifndef APP_RADIO_RETRANSMIT_COUNT_CODE
#error "receiver must document the nRF24 SETUP_RETR.ARC value."
#endif

#ifndef APP_RADIO_ACK_PAYLOAD_REPLACE_AFTER_RX
#error "receiver must document normal RX ACK append policy."
#endif

#ifndef APP_RADIO_ACK_PAYLOAD_REPLACE_ON_RECOVER
#error "receiver must document startup/recovery ACK replace policy."
#endif

static void test_receiver_preloads_short_status_ack_payload_fifo(void)
{
    assert(APP_RADIO_PACKET_SIZE == 32u);
    assert(APP_RADIO_STATUS_ACK_SIZE == 15u);
    assert(APP_RADIO_RATE_CODE == 0u);
    assert(APP_RADIO_POWER_CODE == 3u);
    assert(APP_RADIO_RETRANSMIT_DELAY_CODE == 3u);
    assert(APP_RADIO_RETRANSMIT_COUNT_CODE == 15u);
    assert(APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT == 3u);
    assert(APP_RADIO_ACK_PAYLOAD_REPLACE_AFTER_RX == 0u);
    assert(APP_RADIO_ACK_PAYLOAD_REPLACE_ON_RECOVER == 1u);
}

int main(void)
{
    test_receiver_preloads_short_status_ack_payload_fifo();
    return 0;
}
