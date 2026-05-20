#include "app_radio.h"

#include <assert.h>

#ifndef APP_RADIO_RETRANSMIT_DELAY_CODE
#error "APP_RADIO_RETRANSMIT_DELAY_CODE must document the nRF24 SETUP_RETR.ARD value."
#endif

#ifndef APP_RADIO_RETRANSMIT_COUNT_CODE
#error "APP_RADIO_RETRANSMIT_COUNT_CODE must document the nRF24 SETUP_RETR.ARC value."
#endif

static void test_250kbps_32byte_ack_payload_uses_1500us_retransmit_delay(void)
{
    assert(APP_RADIO_PACKET_SIZE == 32u);
    assert(APP_RADIO_RETRANSMIT_DELAY_CODE == 5u);
    assert(APP_RADIO_RETRANSMIT_COUNT_CODE == 10u);
}

int main(void)
{
    test_250kbps_32byte_ack_payload_uses_1500us_retransmit_delay();
    return 0;
}
