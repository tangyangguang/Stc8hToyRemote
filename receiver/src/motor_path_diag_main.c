#if APP_RECEIVER_MOTOR_PATH_DIAG_MAIN

#include "app_config.h"
#include "app_outputs.h"
#include "app_radio.h"
#include "board_pins.h"
#include "drv_nrf24l01.h"
#include "stc8h_delay.h"
#include "stc8h_spi.h"
#include "toy_remote_protocol.h"

#define MOTOR_PATH_DIAG_STEP_MS 200u
#define MOTOR_PATH_DIAG_STOP_MS 1000u
#define MOTOR_PATH_DIAG_SPEED_STEP 5u
#define MOTOR_PATH_DIAG_ERROR_BLINK_MS 100u

#ifndef MOTOR_PATH_DIAG_BOOT_BLINKS
#define MOTOR_PATH_DIAG_BOOT_BLINKS 1u
#endif

#ifndef MOTOR_PATH_DIAG_INIT_NRF_PINS_BEFORE_OUTPUTS
#define MOTOR_PATH_DIAG_INIT_NRF_PINS_BEFORE_OUTPUTS 0
#endif

#ifndef MOTOR_PATH_DIAG_INIT_NRF_PINS_AFTER_OUTPUTS
#define MOTOR_PATH_DIAG_INIT_NRF_PINS_AFTER_OUTPUTS 0
#endif

#ifndef MOTOR_PATH_DIAG_INIT_SPI
#define MOTOR_PATH_DIAG_INIT_SPI 0
#endif

#ifndef MOTOR_PATH_DIAG_INIT_RADIO
#define MOTOR_PATH_DIAG_INIT_RADIO 0
#endif

#ifndef MOTOR_PATH_DIAG_REINIT_OUTPUTS_AFTER_RADIO
#define MOTOR_PATH_DIAG_REINIT_OUTPUTS_AFTER_RADIO 0
#endif

static STC8H_XDATA toy_remote_control_t control;

static void motor_path_diag_boot_blink(void)
{
    stc8h_u8 i;

    for (i = 0u; i < MOTOR_PATH_DIAG_BOOT_BLINKS; ++i) {
        TOY_REMOTE_RX_LED_ON();
        stc8h_delay_ms(120u);
        TOY_REMOTE_RX_LED_OFF();
        stc8h_delay_ms(120u);
    }
    stc8h_delay_ms(500u);
}

static void motor_path_diag_error_blink(void)
{
    while (1) {
        TOY_REMOTE_RX_LED_ON();
        stc8h_delay_ms(MOTOR_PATH_DIAG_ERROR_BLINK_MS);
        TOY_REMOTE_RX_LED_OFF();
        stc8h_delay_ms(MOTOR_PATH_DIAG_ERROR_BLINK_MS);
    }
}

static void motor_path_diag_apply(stc8h_u8 direction, stc8h_u8 speed)
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

static void motor_path_diag_sweep(stc8h_u8 direction)
{
    stc8h_u8 speed;

    for (speed = 0u; speed < TOY_REMOTE_CONTROL_SPEED_MAX; speed = (stc8h_u8)(speed + MOTOR_PATH_DIAG_SPEED_STEP)) {
        motor_path_diag_apply(direction, speed);
        stc8h_delay_ms(MOTOR_PATH_DIAG_STEP_MS);
    }
    motor_path_diag_apply(direction, TOY_REMOTE_CONTROL_SPEED_MAX);
    stc8h_delay_ms(MOTOR_PATH_DIAG_STEP_MS);
}

void main(void)
{
#if MOTOR_PATH_DIAG_INIT_NRF_PINS_BEFORE_OUTPUTS
    drv_nrf24l01_init_pins();
#endif

#if MOTOR_PATH_DIAG_INIT_SPI
    stc8h_spi_init();
#endif

    app_outputs_init();

#if MOTOR_PATH_DIAG_INIT_NRF_PINS_AFTER_OUTPUTS
    drv_nrf24l01_init_pins();
#endif

    motor_path_diag_boot_blink();

#if MOTOR_PATH_DIAG_INIT_RADIO
    if (app_radio_init_rx(APP_CONFIG_DEFAULT_CHANNEL) != STC8H_OK) {
        app_outputs_apply_safe();
        motor_path_diag_error_blink();
    }
#endif

#if MOTOR_PATH_DIAG_REINIT_OUTPUTS_AFTER_RADIO
    app_outputs_init();
#endif

    while (1) {
        TOY_REMOTE_RX_LED_ON();
        motor_path_diag_sweep(TOY_REMOTE_DIRECTION_FORWARD);
        app_outputs_apply_safe();
        stc8h_delay_ms(MOTOR_PATH_DIAG_STOP_MS);

        TOY_REMOTE_RX_LED_OFF();
        motor_path_diag_sweep(TOY_REMOTE_DIRECTION_REVERSE);
        app_outputs_apply_safe();
        stc8h_delay_ms(MOTOR_PATH_DIAG_STOP_MS);
    }
}

#else
typedef unsigned char motor_path_diag_main_disabled_t;
#endif
