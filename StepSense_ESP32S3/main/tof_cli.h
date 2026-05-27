#ifndef TOF_CLI_H
#define TOF_CLI_H

#include "esp_err.h"

/**
 * @brief Initialize the StepSense CLI
 * This will setup the REPL on the Type-C USB port.
 */
esp_err_t tof_cli_init(void);

#endif // TOF_CLI_H
