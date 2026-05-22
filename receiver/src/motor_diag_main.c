#if APP_RECEIVER_MOTOR_DIAG_MAIN

#include "app_outputs.h"
#include "board_pins.h"
#include "stc8h_delay.h"
#include "toy_remote_protocol.h"

#define MOTOR_DIAG_STEP_MS 200u
#define MOTOR_DIAG_STOP_MS 1000u
#define MOTOR_DIAG_SPEED_STEP 5u

static STC8H_XDATA toy_remote_control_t control;

static void motor_diag_apply(stc8h_u8 direction, stc8h_u8 speed, stc8h_u8 brake)
{
    control.direction = direction;
    control.speed = speed;
    control.brake = brake;
    control.steering_angle = TOY_REMOTE_STEERING_CENTER;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
    control.tx_id = 1u;
    app_outputs_apply_control(&control);
}

static void motor_diag_sweep(stc8h_u8 direction)
{
    stc8h_u8 speed;

    for (speed = 0u; speed < TOY_REMOTE_CONTROL_SPEED_MAX; speed = (stc8h_u8)(speed + MOTOR_DIAG_SPEED_STEP)) {
        motor_diag_apply(direction, speed, 0u);
        stc8h_delay_ms(MOTOR_DIAG_STEP_MS);
    }
    motor_diag_apply(direction, TOY_REMOTE_CONTROL_SPEED_MAX, 0u);
    stc8h_delay_ms(MOTOR_DIAG_STEP_MS);
}

void main(void)
{
    app_outputs_init();

    while (1) {
        TOY_REMOTE_RX_LED_ON();
        motor_diag_sweep(TOY_REMOTE_DIRECTION_FORWARD);

        app_outputs_apply_safe();
        stc8h_delay_ms(MOTOR_DIAG_STOP_MS);

        TOY_REMOTE_RX_LED_OFF();
        motor_diag_sweep(TOY_REMOTE_DIRECTION_REVERSE);

        app_outputs_apply_safe();
        stc8h_delay_ms(MOTOR_DIAG_STOP_MS);
    }
}

#else
typedef unsigned char motor_diag_main_disabled_t;
#endif
