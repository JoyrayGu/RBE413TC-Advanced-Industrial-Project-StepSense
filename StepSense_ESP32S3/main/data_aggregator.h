#ifndef DATA_AGGREGATOR_H
#define DATA_AGGREGATOR_H

#include "tof_protocol.h"

void data_aggregator_init(void);
void data_aggregator_push_frame(const WirelessDataPacket_t *pkt);

#endif
