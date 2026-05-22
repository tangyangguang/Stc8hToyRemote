#if APP_RECEIVER_RADIO_MOTOR_DIAG_MAIN

#include "app_config.h"
#include "app_outputs.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "stc8h_delay.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

#define RADIO_MOTOR_DIAG_STEP_MS 200u
#define RADIO_MOTOR_DIAG_STOP_MS 1000u
#define RADIO_MOTOR_DIAG_SPEED_STEP 5u
#define RADIO_MOTOR_DIAG_ERROR_BLINK_MS 100u

static STC8H_XDATA toy_remote_control_t control;

static void radio_motor_diag_error_blink(void)
{
    while (1) {
        TOY_REMOTE_RX_LED_ON();
        stc8h_delay_ms(RADIO_MOTOR_DIAG_ERROR_BLINK_MS);
        TOY_REMOTE_RX_LED_OFF();
        stc8h_delay_ms(RADIO_MOTOR_DIAG_ERROR_BLINK_MS);
    }
}

static void radio_motor_diag_apply(stc8h_u8 direction, stc8h_u8 speed)
{
    control.direction = direction;
    control.speed = speed;
    control.brake = 0u;
    control.steering_angle = TOY_REMOTE_STEERING_CENTER;
    control.light = 0u;
    control.buzzer = 0u;
    control.aux_pwm = 0u;
    control.request_voltage = 0u;
    control.tx_id = 1u;
    app_outputs_apply_control(&control);
}

static void radio_motor_diag_sweep(stc8h_u8 direction)
{
    stc8h_u8 speed;

    for (speed = 0u; speed < TOY_REMOTE_CONTROL_SPEED_MAX; speed = (stc8h_u8)(speed + RADIO_MOTOR_DIAG_SPEED_STEP)) {
        radio_motor_diag_apply(direction, speed);
        stc8h_delay_ms(RADIO_MOTOR_DIAG_STEP_MS);
    }
    radio_motor_diag_apply(direction, TOY_REMOTE_CONTROL_SPEED_MAX);
    stc8h_delay_ms(RADIO_MOTOR_DIAG_STEP_MS);
}

void main(void)
{
    drv_nrf24l01_init_pins();
    stc8h_spi_init();
    app_outputs_init();

    if (app_radio_init_rx(APP_CONFIG_DEFAULT_CHANNEL) != STC8H_OK) {
        app_outputs_apply_safe();
        radio_motor_diag_error_blink();
    }

    while (1) {
        TOY_REMOTE_RX_LED_ON();
        radio_motor_diag_sweep(TOY_REMOTE_DIRECTION_FORWARD);
        app_outputs_apply_safe();
        stc8h_delay_ms(RADIO_MOTOR_DIAG_STOP_MS);

        TOY_REMOTE_RX_LED_OFF();
        radio_motor_diag_sweep(TOY_REMOTE_DIRECTION_REVERSE);
        app_outputs_apply_safe();
        stc8h_delay_ms(RADIO_MOTOR_DIAG_STOP_MS);
    }
}

#else
typedef unsigned char radio_motor_diag_main_disabled_t;
#endif
