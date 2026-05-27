#ifndef STATUS_MANAGER_H
#define STATUS_MANAGER_H

#include <stdbool.h>
#include "hw_interaction.h"

// Foot identities
#define FOOT_LEFT  0
#define FOOT_RIGHT 1

void status_manager_init(void);
void status_manager_update_foot(int identity);
void status_manager_update_usb(bool open);

// Calculate the recommended LED state based on current metrics
s3_led_state_t status_manager_get_recommended_led(void);

#endif // STATUS_MANAGER_H
