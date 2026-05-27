#ifndef HW_INTERACTION_H
#define HW_INTERACTION_H

#include <stdint.h>

typedef enum {
    S3_LED_RAINBOW,       // Start up / Booting
    S3_LED_CYAN_BREATH,   // Passive listening / auto binding
    S3_LED_YELLOW_PULSE,  // Only one foot connected (Idle)
    S3_LED_YELLOW_ACTIVE, // One foot connected + Data flowing (Fast Pulse)
    S3_LED_GREEN_BREATH,  // Both feet connected + USB Open (Idle)
    S3_LED_GREEN_ACTIVE,  // Both feet connected + Data flowing (Fast Breath)
    S3_LED_GREEN_FLASH,   // Legacy / Overridden
    S3_LED_PURPLE_BREATH, // USB Port not opened by Host
    S3_LED_PURPLE_ACTIVE, // USB Port not opened + Data flowing
    S3_LED_OFF
} s3_led_state_t;

void hw_interaction_init(void);
void hw_interaction_set_led(s3_led_state_t state);
void hw_interaction_trigger_flash(void); // Momentary Green Flash

#endif // HW_INTERACTION_H
