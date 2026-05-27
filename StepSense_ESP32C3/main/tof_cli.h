#ifndef TOF_CLI_H
#define TOF_CLI_H

#include "esp_err.h"

/**
 * @brief Initialize the ESP-IDF console and register StepSense commands
 */
esp_err_t tof_cli_init(void);

#endif // TOF_CLI_H
