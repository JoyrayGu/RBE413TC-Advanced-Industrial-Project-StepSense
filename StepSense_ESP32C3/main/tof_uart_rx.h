#ifndef TOF_UART_RX_H
#define TOF_UART_RX_H

#include "tof_protocol.h"

typedef void (*tof_packet_cb_t)(const UartPacket_t *packet);

void tof_uart_rx_init(int tx_pin, int rx_pin, uint32_t baud_rate);
void tof_uart_rx_set_callback(tof_packet_cb_t cb);

#endif // TOF_UART_RX_H
