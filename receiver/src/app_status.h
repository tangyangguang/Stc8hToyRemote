#ifndef RECEIVER_APP_STATUS_H
#define RECEIVER_APP_STATUS_H

#include "toy_remote_protocol.h"

void app_status_init(toy_remote_status_t *status);
void app_status_update(toy_remote_status_t *status, const toy_remote_control_t *control, stc8h_u8 link_lost);

#endif
