#include "board_pins.h"

#include <assert.h>

volatile unsigned char P3;
volatile unsigned char P1;
volatile unsigned char TOY_REMOTE_NRF24_CSN_BIT;
volatile unsigned char TOY_REMOTE_NRF24_CE_BIT;
volatile unsigned char TOY_REMOTE_RX_MOTOR_IN1_BIT;
volatile unsigned char TOY_REMOTE_RX_MOTOR_IN2_BIT;
volatile unsigned char TOY_REMOTE_RX_LIGHT_BIT;
volatile unsigned char TOY_REMOTE_RX_BUZZER_BIT;
volatile unsigned char TOY_REMOTE_RX_LED_BIT;

static void reset_output_bits(void)
{
    P3 = 0xA5u;
    TOY_REMOTE_RX_MOTOR_IN1_BIT = 1u;
    TOY_REMOTE_RX_MOTOR_IN2_BIT = 1u;
    TOY_REMOTE_RX_LIGHT_BIT = 1u;
    TOY_REMOTE_RX_BUZZER_BIT = 1u;
    TOY_REMOTE_RX_LED_BIT = 0u;
}

static void test_receiver_nrf_control_uses_bit_writes(void)
{
    P1 = 0x5Au;
    TOY_REMOTE_NRF24_CSN_BIT = 0u;
    TOY_REMOTE_NRF24_CE_BIT = 0u;

    DRV_NRF24L01_CSN_HIGH();
    assert(TOY_REMOTE_NRF24_CSN_BIT == 1u);
    assert(P1 == 0x5Au);

    DRV_NRF24L01_CSN_LOW();
    assert(TOY_REMOTE_NRF24_CSN_BIT == 0u);
    assert(P1 == 0x5Au);

    DRV_NRF24L01_CE_HIGH();
    assert(TOY_REMOTE_NRF24_CE_BIT == 1u);
    assert(P1 == 0x5Au);

    DRV_NRF24L01_CE_LOW();
    assert(TOY_REMOTE_NRF24_CE_BIT == 0u);
    assert(P1 == 0x5Au);
}

static void test_receiver_led_is_active_high(void)
{
    reset_output_bits();
    TOY_REMOTE_RX_LED_ON();
    assert(TOY_REMOTE_RX_LED_BIT == 1u);
    assert(P3 == 0xA5u);

    TOY_REMOTE_RX_LED_OFF();
    assert(TOY_REMOTE_RX_LED_BIT == 0u);
    assert(P3 == 0xA5u);
}

static void test_receiver_light_and_buzzer_use_bit_writes(void)
{
    reset_output_bits();
    TOY_REMOTE_RX_LIGHT_ON();
    assert(TOY_REMOTE_RX_LIGHT_BIT == 0u);
    assert(P3 == 0xA5u);

    TOY_REMOTE_RX_LIGHT_OFF();
    assert(TOY_REMOTE_RX_LIGHT_BIT == 1u);
    assert(P3 == 0xA5u);

    TOY_REMOTE_RX_BUZZER_ON();
    assert(TOY_REMOTE_RX_BUZZER_BIT == 0u);
    assert(P3 == 0xA5u);

    TOY_REMOTE_RX_BUZZER_OFF();
    assert(TOY_REMOTE_RX_BUZZER_BIT == 1u);
    assert(P3 == 0xA5u);
}

static void test_receiver_motor_stop_uses_bit_writes(void)
{
    reset_output_bits();
    TOY_REMOTE_RX_MOTOR_STOP();
    assert(TOY_REMOTE_RX_MOTOR_IN1_BIT == 0u);
    assert(TOY_REMOTE_RX_MOTOR_IN2_BIT == 0u);
    assert(P3 == 0xA5u);
}

int main(void)
{
    test_receiver_nrf_control_uses_bit_writes();
    test_receiver_led_is_active_high();
    test_receiver_light_and_buzzer_use_bit_writes();
    test_receiver_motor_stop_uses_bit_writes();
    return 0;
}
