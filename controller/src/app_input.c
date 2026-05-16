#include "app_input.h"
#include "board_pins.h"
#include "drv_ec11.h"
#include "stc8h_adc.h"
#include "stc8h_sfr.h"

#define APP_INPUT_SCAN_MS 1u
#define APP_INPUT_ADC_DIVIDER 16u

static STC8H_XDATA drv_ec11_t speed_encoder;
static stc8h_u8 adc_divider;

void app_input_init(toy_remote_control_t *control)
{
    P1M1 &= (stc8h_u8)~(TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P1M0 &= (stc8h_u8)~(TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P3M1 |= 0x08u;
    P3M0 &= (stc8h_u8)~0x08u;

    drv_ec11_init(&speed_encoder);
    stc8h_adc_init();
    toy_remote_control_set_safe(control);
}

void app_input_update(toy_remote_control_t *control)
{
    stc8h_s16 delta;
    stc8h_u16 adc_value;

    if (control == 0) {
        return;
    }

    drv_ec11_scan(&speed_encoder, TOY_REMOTE_TX_EC11_A_READ(), TOY_REMOTE_TX_EC11_B_READ(), APP_INPUT_SCAN_MS);
    delta = drv_ec11_get_delta(&speed_encoder);
    if (delta != 0) {
        (void)toy_remote_control_adjust_speed(control, delta);
    }

    if (TOY_REMOTE_TX_EC11_SW_ACTIVE() != 0u) {
        (void)toy_remote_control_apply_brake(control, TOY_REMOTE_BRAKE_CLEAR_SPEED);
    } else if (TOY_REMOTE_TX_BRAKE_ACTIVE() != 0u) {
        (void)toy_remote_control_apply_brake(control, TOY_REMOTE_BRAKE_HOLD_SPEED);
    } else {
        (void)toy_remote_control_apply_brake(control, TOY_REMOTE_BRAKE_RELEASE);
    }

    control->direction = TOY_REMOTE_TX_DIR_REVERSE();
    control->light = TOY_REMOTE_TX_LIGHT_ACTIVE();
    control->buzzer = TOY_REMOTE_TX_BUZZER_ACTIVE();
    control->request_voltage = TOY_REMOTE_TX_FN_ACTIVE();

    ++adc_divider;
    if (adc_divider >= APP_INPUT_ADC_DIVIDER) {
        adc_divider = 0u;
        adc_value = stc8h_adc_read(TOY_REMOTE_TX_ADC_STEERING_CHANNEL);
        if (adc_value != STC8H_ADC_INVALID_VALUE) {
            (void)toy_remote_control_set_steering_from_adc(control, adc_value, 0u);
        }
    }
}

stc8h_u16 app_input_read_tx_battery_centivolts(void)
{
    stc8h_u8 i;
    stc8h_u16 adc;
    stc8h_u16 sum;

    sum = 0u;
    for (i = 0u; i < 8u; ++i) {
        adc = stc8h_adc_read(15u);
        if (adc == STC8H_ADC_INVALID_VALUE) {
            return 0u;
        }
        sum = (stc8h_u16)(sum + adc);
    }

    adc = (stc8h_u16)(sum >> 3);
    return (stc8h_u16)(120627u / (stc8h_u16)(adc + 1u));
}
