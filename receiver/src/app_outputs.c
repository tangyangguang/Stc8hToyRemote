#include "app_outputs.h"
#include "board_pins.h"
#include "stc8h_sfr.h"

void app_outputs_apply_safe(void)
{
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
    app_outputs_apply_safe();
}

void app_outputs_apply_control(const toy_remote_control_t *control)
{
    if ((control == 0) || (toy_remote_validate_control(control) != STC8H_OK)) {
        app_outputs_apply_safe();
        return;
    }

    TOY_REMOTE_RX_MOTOR_STOP();

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
