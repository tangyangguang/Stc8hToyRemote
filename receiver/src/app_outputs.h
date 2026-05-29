#ifndef RECEIVER_APP_OUTPUTS_H
#define RECEIVER_APP_OUTPUTS_H

#include "toy_remote_protocol.h"

void app_outputs_init(void);
void app_outputs_apply_control(const STC8H_XDATA toy_remote_control_t *control);
/* Leaves the servo PWM unchanged so link loss holds the last steering angle. */
void app_outputs_apply_safe(void);

#endif
