#include "usb_cdc_bridge.h"
#include <stdio.h>
#include <unistd.h>
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"

void usb_cdc_bridge_init(void) {
    // Driver installed by esp_console (tof_cli)
}

void usb_cdc_bridge_send(const UartPacket_t *packet) {
    usb_serial_jtag_write_bytes((const char*)packet, sizeof(UartPacket_t), 10 / portTICK_PERIOD_MS);
}
