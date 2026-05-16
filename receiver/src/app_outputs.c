#include "app_outputs.h"
#include "board_pins.h"
#include "stc8h_pwm.h"
#include "stc8h_sfr.h"

#define APP_OUTPUT_SERVO_PRESCALER 11u
#define APP_OUTPUT_SERVO_PERIOD 18431u
#define APP_OUTPUT_SERVO_MIN_DUTY 461u
#define APP_OUTPUT_SERVO_MAX_DUTY 2303u
#define APP_OUTPUT_SERVO_CENTER_DUTY 1382u
#define APP_OUTPUT_FAST_PWM_PERIOD 1105u

static stc8h_u16 app_outputs_percent_to_fast_duty(stc8h_u8 percent)
{
    stc8h_u32 duty;

    if (percent > 100u) {
        percent = 100u;
    }

    duty = (stc8h_u32)percent * APP_OUTPUT_FAST_PWM_PERIOD;
    duty = (duty + 50u) / 100u;
    return (stc8h_u16)duty;
}

static stc8h_u16 app_outputs_angle_to_servo_duty(stc8h_u8 angle)
{
    stc8h_u32 duty;

    if (angle > TOY_REMOTE_STEERING_MAX) {
        angle = TOY_REMOTE_STEERING_CENTER;
    }

    duty = (stc8h_u32)(APP_OUTPUT_SERVO_MAX_DUTY - APP_OUTPUT_SERVO_MIN_DUTY) * angle;
    duty = (duty + (TOY_REMOTE_STEERING_MAX / 2u)) / TOY_REMOTE_STEERING_MAX;
    duty += APP_OUTPUT_SERVO_MIN_DUTY;
    return (stc8h_u16)duty;
}

void app_outputs_apply_safe(void)
{
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_6, 0u);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7, 0u);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8, 0u);
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_A, STC8H_PWM_CHANNEL_1, APP_OUTPUT_SERVO_CENTER_DUTY);
    TOY_REMOTE_RX_MOTOR_STOP();
    TOY_REMOTE_RX_LIGHT_OFF();
    TOY_REMOTE_RX_BUZZER_OFF();
    TOY_REMOTE_RX_LED_OFF();
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
    if ((control == 0) || (toy_remote_validate_control(control) != STC8H_OK)) {
        app_outputs_apply_safe();
        return;
    }

    TOY_REMOTE_RX_MOTOR_STOP();
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_A,
                              STC8H_PWM_CHANNEL_1,
                              app_outputs_angle_to_servo_duty(control->steering_angle));
    (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B,
                              STC8H_PWM_CHANNEL_6,
                              app_outputs_percent_to_fast_duty(control->aux_pwm));

    if ((control->brake != 0u) || (control->speed == 0u)) {
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7, 0u);
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8, 0u);
    } else if (control->direction == TOY_REMOTE_DIRECTION_REVERSE) {
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_7, 0u);
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B,
                                  STC8H_PWM_CHANNEL_8,
                                  app_outputs_percent_to_fast_duty(control->speed));
    } else {
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B,
                                  STC8H_PWM_CHANNEL_7,
                                  app_outputs_percent_to_fast_duty(control->speed));
        (void)stc8h_pwm_set_duty(STC8H_PWM_GROUP_B, STC8H_PWM_CHANNEL_8, 0u);
    }

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

    TOY_REMOTE_RX_LED_ON();
}
