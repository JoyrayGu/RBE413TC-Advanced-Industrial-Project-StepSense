#ifndef USB_CDC_BRIDGE_H
#define USB_CDC_BRIDGE_H
#include "tof_protocol.h"

void usb_cdc_bridge_init(void);
void usb_cdc_bridge_send(const UartPacket_t *packet);

#endif // USB_CDC_BRIDGE_H
