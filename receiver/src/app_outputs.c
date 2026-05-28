#include "app_outputs.h"
#include "app_outputs_calc.h"
#include "board_pins.h"
#include "stc8h_pwm.h"
#include "stc8h_sfr.h"

static void app_outputs_write_pwm(stc8h_u16 servo, stc8h_u16 aux, stc8h_u16 fwd, stc8h_u16 rev)
{
    stc8h_pwm_set_duty_a1(servo);
    stc8h_pwm_set_duty_b6(aux);
    stc8h_pwm_set_duty_b7(fwd);
    stc8h_pwm_set_duty_b8(rev);
}

void app_outputs_apply_safe(void)
{
    stc8h_pwm_set_duty_b6(0u);
    stc8h_pwm_set_duty_b7(0u);
    stc8h_pwm_set_duty_b8(0u);
    TOY_REMOTE_RX_MOTOR_STOP();
    TOY_REMOTE_RX_LIGHT_OFF();
    TOY_REMOTE_RX_BUZZER_OFF();
}

void app_outputs_init(void)
{
    P3M0 |= (stc8h_u8)(TOY_REMOTE_RX_MOTOR_IN1_MASK |
                       TOY_REMOTE_RX_MOTOR_IN2_MASK |
                       TOY_REMOTE_RX_LIGHT_MASK |
                       TOY_REMOTE_RX_BUZZER_MASK |
                       TOY_REMOTE_RX_LED_MASK);
    P3M1 &= (stc8h_u8)~(TOY_REMOTE_RX_MOTOR_IN1_MASK |
                        TOY_REMOTE_RX_MOTOR_IN2_MASK |
                        TOY_REMOTE_RX_LIGHT_MASK |
                        TOY_REMOTE_RX_BUZZER_MASK |
                        TOY_REMOTE_RX_LED_MASK);

    P1M0 |= 0x01u;
    P1M1 &= (stc8h_u8)~0x01u;
    P5M0 |= 0x10u;
    P5M1 &= (stc8h_u8)~0x10u;

    stc8h_pwm_set_prescaler_a(APP_OUTPUT_SERVO_PRESCALER);
    stc8h_pwm_set_period_a(APP_OUTPUT_SERVO_PERIOD);
    stc8h_pwm_init_a1(STC8H_PWM_PIN_PWM1_P10);
    stc8h_pwm_set_duty_a1(APP_OUTPUT_SERVO_CENTER_DUTY);
    stc8h_pwm_enable_a1();

    stc8h_pwm_set_prescaler_b(APP_OUTPUT_FAST_PWM_PRESCALER);
    stc8h_pwm_set_period_b(APP_OUTPUT_FAST_PWM_PERIOD);
    stc8h_pwm_init_b6(STC8H_PWM_PIN_PWM6_P54);
    stc8h_pwm_init_b7(STC8H_PWM_PIN_PWM7_P33);
    stc8h_pwm_init_b8(STC8H_PWM_PIN_PWM8_P34);
    stc8h_pwm_enable_b6();
    stc8h_pwm_enable_b7();
    stc8h_pwm_enable_b8();

    app_outputs_apply_safe();
}

void app_outputs_apply_control(const toy_remote_control_t *control)
{
    stc8h_u16 fwd_duty;
    stc8h_u16 rev_duty;

    APP_OUTPUT_SET_MOTOR_DUTIES(control->direction, control->speed, control->brake, fwd_duty, rev_duty);
    app_outputs_write_pwm(APP_OUTPUT_SERVO_DUTY(control->steering_angle),
                          APP_OUTPUT_FAST_DUTY(control->aux_pwm),
                          fwd_duty,
                          rev_duty);

    if (control->light != 0u) {
        TOY_REMOTE_RX_LIGHT_ON();
    } else {
        TOY_REMOTE_RX_LIGHT_OFF();
    }

    if (control->buzzer != 0u) {
        TOY_REMOTE_RX_BUZZER_ON();
    } else {
        TOY_REMOTE_RX_BUZZER_OFF();
    }

}
