#ifndef HW_INTERACTION_H
#define HW_INTERACTION_H

typedef enum {
    LED_STATE_RAINBOW,           // Power-on
    LED_STATE_YELLOW_BLINK,      // DISCOVERY (Binding)
    LED_STATE_GREEN_FAST_BLINK, // LOCKED (Standby/Connected)
    LED_STATE_GREEN_BREATHE,    // DATA STREAMING
    LED_STATE_GREEN_FLASH,       // Handshake Trigger (visual cue)
    LED_STATE_RED_SLOW,          // Disconnect
    LED_STATE_RED_SOLID          // Fault
} led_state_t;

void hw_interaction_init(void);
void hw_interaction_set_led(led_state_t state);

#endif // HW_INTERACTION_H
