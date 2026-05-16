#ifndef RECEIVER_APP_OUTPUTS_H
#define RECEIVER_APP_OUTPUTS_H

#include "toy_remote_protocol.h"

void app_outputs_init(void);
void app_outputs_apply_control(const toy_remote_control_t *control);
void app_outputs_apply_safe(void);

#endif
