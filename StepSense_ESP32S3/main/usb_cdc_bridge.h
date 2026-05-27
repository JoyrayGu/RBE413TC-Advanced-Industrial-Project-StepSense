#ifndef USB_CDC_BRIDGE_H
#define USB_CDC_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void usb_cdc_bridge_init(void);
void usb_cdc_bridge_send(const uint8_t *data, size_t len);
bool usb_cdc_is_connected(void);

#endif // USB_CDC_BRIDGE_H
