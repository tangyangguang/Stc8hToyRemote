#include "app_radio.h"

#include <assert.h>

#ifndef APP_RADIO_RETRANSMIT_DELAY_CODE
#error "APP_RADIO_RETRANSMIT_DELAY_CODE must document the nRF24 SETUP_RETR.ARD value."
#endif

#ifndef APP_RADIO_RETRANSMIT_COUNT_CODE
#error "APP_RADIO_RETRANSMIT_COUNT_CODE must document the nRF24 SETUP_RETR.ARC value."
#endif

#ifndef APP_RADIO_STATUS_ACK_SIZE
#error "APP_RADIO_STATUS_ACK_SIZE must document the nRF24 ACK payload length."
#endif

#ifndef APP_RADIO_RATE_CODE
#error "APP_RADIO_RATE_CODE must document the nRF24 RF data rate."
#endif

#ifndef APP_RADIO_POWER_CODE
#error "APP_RADIO_POWER_CODE must document the nRF24 RF output power."
#endif

static void test_status_ack_payload_uses_distance_priority_rf_settings(void)
{
    assert(APP_RADIO_PACKET_SIZE == 32u);
    assert(APP_RADIO_STATUS_ACK_SIZE == 15u);
    assert(APP_RADIO_RATE_CODE == 0u);
    assert(APP_RADIO_POWER_CODE == 3u);
    assert(APP_RADIO_RETRANSMIT_DELAY_CODE == 3u);
    assert(APP_RADIO_RETRANSMIT_COUNT_CODE == 15u);
}

static void test_tx_results_classify_radio_ack_payload_state(void)
{
    assert(APP_RADIO_TX_DONE != APP_RADIO_TX_ACK_PAYLOAD_OK);
    assert(APP_RADIO_TX_ACK_PAYLOAD_OK != APP_RADIO_TX_ACK_EMPTY);
    assert(APP_RADIO_TX_ACK_EMPTY != APP_RADIO_TX_ACK_BAD);
    assert(APP_RADIO_TX_MAX_RETRY != APP_RADIO_TX_ACK_BAD);
}

int main(void)
{
    test_status_ack_payload_uses_distance_priority_rf_settings();
    test_tx_results_classify_radio_ack_payload_state();
    return 0;
}
