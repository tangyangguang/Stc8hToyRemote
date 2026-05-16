#ifndef TOY_REMOTE_PROTOCOL_H
#define TOY_REMOTE_PROTOCOL_H

#include "stc8h_config.h"

#define TOY_REMOTE_PROTOCOL_VERSION 1u

#define TOY_REMOTE_CONTROL_PAYLOAD_SIZE 9u
#define TOY_REMOTE_STATUS_PAYLOAD_SIZE 4u

#define TOY_REMOTE_DIRECTION_FORWARD 0u
#define TOY_REMOTE_DIRECTION_REVERSE 1u

#define TOY_REMOTE_CONTROL_SPEED_MAX 100u
#define TOY_REMOTE_CONTROL_AUX_PWM_MAX 100u
#define TOY_REMOTE_STEERING_MIN 0u
#define TOY_REMOTE_STEERING_CENTER 90u
#define TOY_REMOTE_STEERING_MAX 180u
#define TOY_REMOTE_STEERING_ADC_MAX 1023u

#define TOY_REMOTE_LINK_STATE_IDLE 0u
#define TOY_REMOTE_LINK_STATE_CONNECTING 1u
#define TOY_REMOTE_LINK_STATE_CONNECTED 2u
#define TOY_REMOTE_LINK_STATE_LOST 3u

#define TOY_REMOTE_VOLTAGE_INT_MAX 99u
#define TOY_REMOTE_VOLTAGE_DEC_MAX 99u

#define TOY_REMOTE_BRAKE_RELEASE 0u
#define TOY_REMOTE_BRAKE_HOLD_SPEED 1u
#define TOY_REMOTE_BRAKE_CLEAR_SPEED 2u

#ifndef TOY_REMOTE_ENABLE_CONTROL_SET_SAFE
#define TOY_REMOTE_ENABLE_CONTROL_SET_SAFE 1
#endif

#ifndef TOY_REMOTE_ENABLE_CONTROL_APPLY_BRAKE
#define TOY_REMOTE_ENABLE_CONTROL_APPLY_BRAKE 1
#endif

#ifndef TOY_REMOTE_ENABLE_CONTROL_SET_STEERING_FROM_ADC
#define TOY_REMOTE_ENABLE_CONTROL_SET_STEERING_FROM_ADC 1
#endif

#ifndef TOY_REMOTE_ENABLE_CONTROL_ADJUST_SPEED
#define TOY_REMOTE_ENABLE_CONTROL_ADJUST_SPEED 1
#endif

#ifndef TOY_REMOTE_ENABLE_STATUS_SET_VOLTAGE
#define TOY_REMOTE_ENABLE_STATUS_SET_VOLTAGE 1
#endif

#ifndef TOY_REMOTE_ENABLE_VALIDATE_CONTROL
#define TOY_REMOTE_ENABLE_VALIDATE_CONTROL 1
#endif

#ifndef TOY_REMOTE_ENABLE_VALIDATE_STATUS
#define TOY_REMOTE_ENABLE_VALIDATE_STATUS 1
#endif

#ifndef TOY_REMOTE_ENABLE_PACK_CONTROL
#define TOY_REMOTE_ENABLE_PACK_CONTROL 1
#endif

#ifndef TOY_REMOTE_ENABLE_UNPACK_CONTROL
#define TOY_REMOTE_ENABLE_UNPACK_CONTROL 1
#endif

#ifndef TOY_REMOTE_ENABLE_PACK_STATUS
#define TOY_REMOTE_ENABLE_PACK_STATUS 1
#endif

#ifndef TOY_REMOTE_ENABLE_UNPACK_STATUS
#define TOY_REMOTE_ENABLE_UNPACK_STATUS 1
#endif

typedef struct {
    stc8h_u8 direction;
    stc8h_u8 speed;
    stc8h_u8 brake;
    stc8h_u8 steering_angle;
    stc8h_u8 light;
    stc8h_u8 buzzer;
    stc8h_u8 aux_pwm;
    stc8h_u8 request_voltage;
} toy_remote_control_t;

typedef struct {
    stc8h_u8 link_state;
    stc8h_u8 voltage_int;
    stc8h_u8 voltage_dec;
} toy_remote_status_t;

#if TOY_REMOTE_ENABLE_CONTROL_SET_SAFE
void toy_remote_control_set_safe(toy_remote_control_t *control);
#endif
#if TOY_REMOTE_ENABLE_CONTROL_APPLY_BRAKE
stc8h_status_t toy_remote_control_apply_brake(toy_remote_control_t *control, stc8h_u8 brake_action);
#endif
#if TOY_REMOTE_ENABLE_CONTROL_SET_STEERING_FROM_ADC
stc8h_status_t toy_remote_control_set_steering_from_adc(toy_remote_control_t *control, stc8h_u16 adc_value, stc8h_u8 reverse);
#endif
#if TOY_REMOTE_ENABLE_CONTROL_ADJUST_SPEED
stc8h_status_t toy_remote_control_adjust_speed(toy_remote_control_t *control, stc8h_s16 delta);
#endif
#if TOY_REMOTE_ENABLE_STATUS_SET_VOLTAGE
stc8h_status_t toy_remote_status_set_voltage_centivolts(toy_remote_status_t *status, stc8h_u16 centivolts);
#endif
#if TOY_REMOTE_ENABLE_VALIDATE_CONTROL
stc8h_status_t toy_remote_validate_control(const toy_remote_control_t *control);
#endif
#if TOY_REMOTE_ENABLE_VALIDATE_STATUS
stc8h_status_t toy_remote_validate_status(const toy_remote_status_t *status);
#endif

#if TOY_REMOTE_ENABLE_PACK_CONTROL
stc8h_status_t toy_remote_pack_control(stc8h_u8 *payload, const toy_remote_control_t *control);
#endif
#if TOY_REMOTE_ENABLE_UNPACK_CONTROL
stc8h_status_t toy_remote_unpack_control(toy_remote_control_t *control, const stc8h_u8 *payload, stc8h_u8 len);
#endif
#if TOY_REMOTE_ENABLE_PACK_STATUS
stc8h_status_t toy_remote_pack_status(stc8h_u8 *payload, const toy_remote_status_t *status);
#endif
#if TOY_REMOTE_ENABLE_UNPACK_STATUS
stc8h_status_t toy_remote_unpack_status(toy_remote_status_t *status, const stc8h_u8 *payload, stc8h_u8 len);
#endif

#endif
