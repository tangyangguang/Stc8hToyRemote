#include "app_outputs.h"
#include "app_outputs_calc.h"
#include "board_pins.h"
#include "stc8h_pwm.h"
#include "stc8h_sfr.h"

static void app_outputs_write_pwm(stc8h_u16 servo, stc8h_u16 aux, stc8h_u16 fwd, stc8h_u16 rev)
{
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_A, STC8H_PWM_CHANNEL_1, servo);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_6, aux);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7, fwd);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8, rev);
}

void app_outputs_apply_safe(void)
{
    app_outputs_write_pwm(APP_OUTPUT_SERVO_CENTER_DUTY, 0u, 0u, 0u);
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

    (void)stc8h_pwm_set_prescaler(STC8H_PWM_GROUP_A, APP_OUTPUT_SERVO_PRESCALER);
    (void)stc8h_pwm_set_period(STC8H_PWM_GROUP_A, APP_OUTPUT_SERVO_PERIOD);
    (void)stc8h_pwm_init_channel(STC8H_PWM_GROUP_A, STC8H_PWM_CHANNEL_1, STC8H_PWM_PIN_PWM1_P10);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_A, STC8H_PWM_CHANNEL_1, APP_OUTPUT_SERVO_CENTER_DUTY);
    (void)stc8h_pwm_enable(STC8H_PWM_GROUP_A, STC8H_PWM_CHANNEL_1);

    (void)stc8h_pwm_set_prescaler(STC8H_PWM_GROUP_B, APP_OUTPUT_FAST_PWM_PRESCALER);
    (void)stc8h_pwm_set_period(STC8H_PWM_GROUP_B, APP_OUTPUT_FAST_PWM_PERIOD);
    (void)stc8h_pwm_init_channel(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_6, STC8H_PWM_PIN_PWM6_P54);
    (void)stc8h_pwm_init_channel(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7, STC8H_PWM_PIN_PWM7_P33);
    (void)stc8h_pwm_init_channel(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8, STC8H_PWM_PIN_PWM8_P34);
    (void)stc8h_pwm_enable(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_6);
    (void)stc8h_pwm_enable(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7);
    (void)stc8h_pwm_enable(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8);

    app_outputs_apply_safe();
}

void app_outputs_apply_control(const toy_remote_control_t *control)
{
    stc8h_u16 aux_duty;
    stc8h_u16 fwd_duty;
    stc8h_u16 rev_duty;

    aux_duty = APP_OUTPUT_MOTOR_SUPPLY_DUTY(control->speed, control->brake);
    APP_OUTPUT_SET_MOTOR_DUTIES(control->direction, control->speed, control->brake, fwd_duty, rev_duty);
    app_outputs_write_pwm(APP_OUTPUT_SERVO_DUTY(control->steering_angle),
                          aux_duty,
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
