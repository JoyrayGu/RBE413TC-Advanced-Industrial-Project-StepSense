#ifndef ESPNOW_RX_H
#define ESPNOW_RX_H

#include <stdint.h>
#include <stdbool.h>
#include "tof_protocol.h"

void espnow_rx_init(void);

/**
 * @brief Get currently paired foot units
 */
int espnow_rx_get_peers(uint8_t macs[2][6]);

#endif // ESPNOW_RX_H
