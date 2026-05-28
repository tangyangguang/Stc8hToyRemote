#define APP_EC11_SPEED_ENABLE_SCALE_HELPER 0
#include "app_ec11_speed.h"
#include "app_input.h"
#include "board_pins.h"
#include "drv_ec11.h"
#include "stc8h_adc.h"
#include "stc8h_sfr.h"

#ifndef APP_INPUT_DIAG_DISPLAY
#define APP_INPUT_DIAG_DISPLAY 0
#endif

#define APP_INPUT_ADC_DIVIDER 16u
#define APP_INPUT_P3_BUTTON_MASK (TOY_REMOTE_TX_BRAKE_MASK | TOY_REMOTE_TX_FN_MASK | \
                                  TOY_REMOTE_TX_BUZZER_MASK | TOY_REMOTE_TX_LIGHT_MASK | \
                                  TOY_REMOTE_TX_DIR_MASK)

static STC8H_DATA drv_ec11_small_t speed_encoder;
static volatile STC8H_XDATA stc8h_s16 speed_encoder_delta_accum;
static volatile STC8H_XDATA stc8h_u16 speed_encoder_tick_half_ms;
static volatile STC8H_XDATA stc8h_u16 speed_encoder_last_step_tick_half_ms;
static volatile stc8h_u8 speed_encoder_step_tick_valid;
static volatile stc8h_u8 speed_encoder_accel_enabled;
static stc8h_u8 adc_divider;

void app_input_init(toy_remote_control_t *control)
{
    P1M1 &= (stc8h_u8)~(TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P1M0 &= (stc8h_u8)~(TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P3M1 &= (stc8h_u8)~APP_INPUT_P3_BUTTON_MASK;
    P3M0 &= (stc8h_u8)~APP_INPUT_P3_BUTTON_MASK;
    P3M1 |= 0x08u;
    P3M0 &= (stc8h_u8)~0x08u;
    P5M1 &= (stc8h_u8)~TOY_REMOTE_TX_EC11_SW_MASK;
    P5M0 &= (stc8h_u8)~TOY_REMOTE_TX_EC11_SW_MASK;
    P_SW2 |= 0x80u;
    P1IE |= (TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P1PU |= (TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK);
    P3IE |= APP_INPUT_P3_BUTTON_MASK;
    P3PU |= APP_INPUT_P3_BUTTON_MASK;
    P3IE &= (stc8h_u8)~0x08u;
    P3PU &= (stc8h_u8)~0x08u;
    P5IE |= TOY_REMOTE_TX_EC11_SW_MASK;
    P5PU |= TOY_REMOTE_TX_EC11_SW_MASK;

    drv_ec11_small_init_isr(&speed_encoder);
    speed_encoder_delta_accum = 0;
    speed_encoder_tick_half_ms = 0u;
    speed_encoder_last_step_tick_half_ms = 0u;
    speed_encoder_step_tick_valid = 0u;
    speed_encoder_accel_enabled = 1u;
#if APP_INPUT_DIAG_DISPLAY
    speed_encoder.last_state = (stc8h_u8)(P1 & (TOY_REMOTE_TX_EC11_A_MASK | TOY_REMOTE_TX_EC11_B_MASK));
#endif
    stc8h_adc_init();
    control->direction = TOY_REMOTE_DIRECTION_FORWARD;
    control->speed = 0u;
    control->brake = 0u;
    control->steering_angle = TOY_REMOTE_STEERING_CENTER;
    control->light = 0u;
    control->buzzer = 0u;
    control->aux_pwm = 0u;
    control->request_voltage = 0u;
    control->tx_id = 0u;
}

void app_input_set_speed_accel_enabled(stc8h_u8 enabled)
{
    EA = 0;
    speed_encoder_accel_enabled = enabled;
    speed_encoder_step_tick_valid = 0u;
    EA = 1;
}

void app_input_encoder_tick_isr(void)
{
    stc8h_s16 delta;
    stc8h_u16 interval_half_ms;

    ++speed_encoder_tick_half_ms;
    delta = drv_ec11_scan_delta_small_isr(&speed_encoder, TOY_REMOTE_TX_EC11_A_READ(), TOY_REMOTE_TX_EC11_B_READ());
    if (delta != 0) {
        if (speed_encoder_step_tick_valid == 0u) {
            interval_half_ms = 0xFFFFu;
            speed_encoder_step_tick_valid = 1u;
        } else {
            interval_half_ms = (stc8h_u16)(speed_encoder_tick_half_ms - speed_encoder_last_step_tick_half_ms);
        }
        speed_encoder_last_step_tick_half_ms = speed_encoder_tick_half_ms;
        if ((speed_encoder_accel_enabled != 0u) && (interval_half_ms <= APP_EC11_SPEED_MEDIUM_HALF_MS)) {
            delta = (delta > 0) ? 5 : -5;
        }
        speed_encoder_delta_accum = (stc8h_s16)(speed_encoder_delta_accum + delta);
    }
}

stc8h_u16 app_input_tick_half_ms(void)
{
    stc8h_u16 tick;

    EA = 0;
    tick = speed_encoder_tick_half_ms;
    EA = 1;
    return tick;
}

stc8h_s16 app_input_update(toy_remote_control_t *control)
{
    stc8h_s16 delta;
    stc8h_u16 adc_value;

    EA = 0;
    delta = speed_encoder_delta_accum;
    speed_encoder_delta_accum = 0;
    EA = 1;

    if (delta != 0) {
        adc_value = (stc8h_u16)((stc8h_s16)control->speed + delta);
        if (((stc8h_s16)adc_value) < 0) {
            control->speed = 0u;
        } else if (adc_value > TOY_REMOTE_CONTROL_SPEED_MAX) {
            control->speed = TOY_REMOTE_CONTROL_SPEED_MAX;
        } else {
            control->speed = (stc8h_u8)adc_value;
        }
    }

    if (TOY_REMOTE_TX_EC11_SW_ACTIVE() != 0u) {
        control->brake = 1u;
        control->speed = 0u;
    } else if (TOY_REMOTE_TX_BRAKE_ACTIVE() != 0u) {
        control->brake = 1u;
    } else {
        control->brake = 0u;
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
            if (adc_value > TOY_REMOTE_STEERING_ADC_MAX) {
                adc_value = TOY_REMOTE_STEERING_ADC_MAX;
            }
            adc_value = (stc8h_u16)(((adc_value * 45u) + 128u) >> 8);
            control->steering_angle = (adc_value > TOY_REMOTE_STEERING_MAX) ?
                TOY_REMOTE_STEERING_MAX : (stc8h_u8)adc_value;
        }
    }
    return delta;
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

    adc = (stc8h_u16)((sum >> 3) + 1u);
    if (adc < 5u) {
        return 9999u;
    }
    return (stc8h_u16)(12063u / (stc8h_u16)((adc + 5u) / 10u));
}
