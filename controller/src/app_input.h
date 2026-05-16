#ifndef CONTROLLER_APP_INPUT_H
#define CONTROLLER_APP_INPUT_H

#include "toy_remote_protocol.h"

void app_input_init(toy_remote_control_t *control);
void app_input_update(toy_remote_control_t *control);

#endif
