#include "app_status.h"
#include "board_pins.h"
#include "stc8h_adc.h"

#include <assert.h>

volatile unsigned char P1M0;
volatile unsigned char P1M1;

static stc8h_u16 test_adc_value;
static stc8h_u8 test_adc_read_count;

void stc8h_adc_init(void)
{
}

stc8h_u16 stc8h_adc_read(stc8h_u8 channel)
{
    assert(channel == TOY_REMOTE_RX_ADC_VOLTAGE_CHANNEL);
    ++test_adc_read_count;
    return test_adc_value;
}

static void test_init_configures_voltage_adc_pin_high_impedance(void)
{
    toy_remote_status_t status;

    P1M0 = 0xFFu;
    P1M1 = 0x00u;

    app_status_init(&status);

    assert((P1M0 & 0x02u) == 0u);
    assert((P1M1 & 0x02u) == 0x02u);
}

static void test_requested_voltage_uses_centivolt_scale(void)
{
    toy_remote_status_t status;
    toy_remote_control_t control;
    stc8h_u8 i;

    app_status_init(&status);
    test_adc_value = 572u;
    test_adc_read_count = 0u;

    control.request_voltage = 1u;
    for (i = 0u; i < 5u; ++i) {
        app_status_update(&status, &control, 0u);
        assert(status.voltage_int == 0u);
        assert(status.voltage_dec == 0u);
    }

    app_status_update(&status, &control, 0u);

    assert(test_adc_read_count == 8u);
    assert(status.voltage_int == 7u);
    assert(status.voltage_dec == 42u);
}

static void test_idle_voltage_samples_about_once_per_second(void)
{
    toy_remote_status_t status;
    toy_remote_control_t control;
    stc8h_u8 i;

    app_status_init(&status);
    test_adc_value = 572u;
    test_adc_read_count = 0u;

    control.request_voltage = 0u;
    for (i = 0u; i < 49u; ++i) {
        app_status_update(&status, &control, 0u);
        assert(test_adc_read_count == 0u);
        assert(status.voltage_int == 0u);
        assert(status.voltage_dec == 0u);
    }

    app_status_update(&status, &control, 0u);

    assert(test_adc_read_count == 8u);
    assert(status.voltage_int == 7u);
    assert(status.voltage_dec == 42u);
}

int main(void)
{
    test_requested_voltage_uses_centivolt_scale();
    test_idle_voltage_samples_about_once_per_second();
    test_init_configures_voltage_adc_pin_high_impedance();
    return 0;
}
