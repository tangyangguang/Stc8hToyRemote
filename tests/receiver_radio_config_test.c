#include "app_radio.h"

#include <assert.h>

#ifndef APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT
#error "receiver must document how many ACK payloads are preloaded."
#endif

static void test_receiver_preloads_full_ack_payload_fifo(void)
{
    assert(APP_RADIO_PACKET_SIZE == 32u);
    assert(APP_RADIO_ACK_PAYLOAD_PRELOAD_COUNT == 3u);
}

int main(void)
{
    test_receiver_preloads_full_ack_payload_fifo();
    return 0;
}
