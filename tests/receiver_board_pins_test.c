#include "board_pins.h"

#include <assert.h>

volatile unsigned char P3;

static void test_receiver_led_is_active_high(void)
{
    P3 = 0u;
    TOY_REMOTE_RX_LED_ON();
    assert((P3 & TOY_REMOTE_RX_LED_MASK) != 0u);

    TOY_REMOTE_RX_LED_OFF();
    assert((P3 & TOY_REMOTE_RX_LED_MASK) == 0u);
}

int main(void)
{
    test_receiver_led_is_active_high();
    return 0;
}
