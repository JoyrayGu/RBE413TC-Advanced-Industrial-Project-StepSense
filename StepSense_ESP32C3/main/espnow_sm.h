#ifndef ESPNOW_SM_H
#define ESPNOW_SM_H

#include "tof_protocol.h"

void espnow_sm_init(void);
void espnow_sm_send_data(const UartPacket_t *packet);

#endif // ESPNOW_SM_H
