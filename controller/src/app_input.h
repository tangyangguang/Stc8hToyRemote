#ifndef CONTROLLER_APP_INPUT_H
#define CONTROLLER_APP_INPUT_H

#include "toy_remote_protocol.h"

void app_input_init(toy_remote_control_t *control);
void app_input_set_speed_accel_enabled(stc8h_u8 enabled);
void app_input_encoder_tick_isr(void);
stc8h_u16 app_input_tick_half_ms(void);
void app_input_update_steering(toy_remote_control_t *control);
stc8h_s16 app_input_update(toy_remote_control_t *control);
stc8h_u16 app_input_read_tx_battery_centivolts(void);

#endif
