#include "sdkconfig.h"

// Force override any sdkconfig UART defaults to use the Type-C JTAG Console
#undef CONFIG_ESP_CONSOLE_UART_DEFAULT
#define CONFIG_ESP_CONSOLE_UART_DEFAULT 0
#undef CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#define CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG 1

#include "tof_cli.h"
#include "tof_settings.h"
#include "tof_protocol.h"
#include "esp_console.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include <stdio.h>
#include <string.h>
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"

#include "esp_mac.h"

/* --- 1. show_cfg command --- */
static int do_show_cfg(int argc, char **argv) {
    TofSettings_t *s = tof_settings_get();
    
    uint8_t local_mac[6];
    esp_read_mac(local_mac, ESP_MAC_WIFI_STA);

    printf("\n--- StepSense Configurations ---\n");
    printf("Local MAC:     %02X:%02X:%02X:%02X:%02X:%02X\n",
           local_mac[0], local_mac[1], local_mac[2],
           local_mac[3], local_mac[4], local_mac[5]);
    const char *ident_str = "UNKNOWN";
    if (s->identity == IDENTITY_LEFT) ident_str = "LEFT";
    else if (s->identity == IDENTITY_RIGHT) ident_str = "RIGHT";
    else if (s->identity == IDENTITY_UNASSIGNED) ident_str = "NULL (Unassigned)";

    printf("Identity:      %s\n", ident_str);
    printf("WiFi Channel:  %d\n", s->wifi_ch);
    printf("UART Baud:     %u\n", (unsigned int)s->uart_baud);
    printf("Peer MAC:      %02X:%02X:%02X:%02X:%02X:%02X\n",
           s->peer_mac[0], s->peer_mac[1], s->peer_mac[2],
           s->peer_mac[3], s->peer_mac[4], s->peer_mac[5]);
    printf("Mirror Mode:   %s\n", s->mirror_enabled ? "ON (Binary Passthrough)" : "OFF");
    printf("--------------------------------\n");
    return 0;
}

/* --- 1.5 restart command --- */
static int do_restart(int argc, char **argv) {
    printf("Restarting system...\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return 0;
}

/* --- 2. set_identity command --- */
static int do_set_identity(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: set_identity <L/R>\n");
        return 1;
    }
    TofSettings_t *s = tof_settings_get();
    if (strcasecmp(argv[1], "L") == 0) s->identity = IDENTITY_LEFT;
    else if (strcasecmp(argv[1], "R") == 0) s->identity = IDENTITY_RIGHT;
    else {
        printf("Invalid identity. Use L or R.\n");
        return 1;
    }
    tof_settings_save();
    printf("Identity set to %s. Please restart.\n", s->identity == IDENTITY_LEFT ? "LEFT" : "RIGHT");
    return 0;
}

/* --- 3. set_mirror command --- */
static int do_set_mirror(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: set_mirror <ON/OFF>\n");
        return 1;
    }
    TofSettings_t *s = tof_settings_get();
    if (strcasecmp(argv[1], "ON") == 0) s->mirror_enabled = 1;
    else if (strcasecmp(argv[1], "OFF") == 0) s->mirror_enabled = 0;
    else {
        printf("Invalid value. Use ON or OFF.\n");
        return 1;
    }
    tof_settings_save();
    printf("Mirror mode set to %s.\n", s->mirror_enabled ? "ON" : "OFF");
    
    // Dynamically silence logs if mirroring is enabled to prevent stream corruption
    if (s->mirror_enabled) {
        esp_log_level_set("*", ESP_LOG_NONE);
    } else {
        esp_log_level_set("*", ESP_LOG_INFO);
    }
    return 0;
}

/* --- 4. set_wireless command --- */
static struct {
    struct arg_int *ch;
    struct arg_str *peer;
    struct arg_end *end;
} wireless_args;

static int do_set_wireless(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&wireless_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, wireless_args.end, argv[0]);
        return 1;
    }

    TofSettings_t *s = tof_settings_get();
    if (wireless_args.ch->count > 0) {
        s->wifi_ch = (uint8_t)wireless_args.ch->ival[0];
    }
    
    if (wireless_args.peer->count > 0) {
        if (sscanf(wireless_args.peer->sval[0], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
                   &s->peer_mac[0], &s->peer_mac[1], &s->peer_mac[2], 
                   &s->peer_mac[3], &s->peer_mac[4], &s->peer_mac[5]) != 6) {
            printf("Invalid MAC format. Use XX:XX:XX:XX:XX:XX\n");
            return 1;
        }
    }

    tof_settings_save();
    printf("Wireless settings updated.\n");
    return 0;
}

#define CONSOLE_BAUD_RATE 2000000

esp_err_t tof_cli_init(void) {
    // 1. Setup REPL config
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "StepSense> ";
    repl_config.max_history_len = 10;

    // 2. Initialize REPL based on hardware
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    printf("\n[CLI] Configured via USB-Serial-JTAG (Type-C)\n");
#else
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    hw_config.baud_rate = CONSOLE_BAUD_RATE;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    printf("\n[CLI] Configured via UART0 (%u bps)\n", (unsigned int)hw_config.baud_rate);
#endif

    // 3. Register show_cfg
    const esp_console_cmd_t show_cmd = {
        .command = "show_cfg",
        .help = "Show current NVS configurations",
        .hint = NULL,
        .func = &do_show_cfg,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&show_cmd));

    // 4. Register set_identity
    const esp_console_cmd_t ident_cmd = {
        .command = "set_identity",
        .help = "Set foot identity (L/R)",
        .hint = "<L/R>",
        .func = &do_set_identity,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&ident_cmd));

    // Register restart
    const esp_console_cmd_t restart_cmd = {
        .command = "restart",
        .help = "Restart the system",
        .func = &do_restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&restart_cmd));

    // Register set_mirror
    const esp_console_cmd_t mirror_cmd = {
        .command = "set_mirror",
        .help = "Toggle raw data mirroring to PC (ON/OFF)",
        .hint = "<ON/OFF>",
        .func = &do_set_mirror,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&mirror_cmd));

    // 5. Register set_wireless
    wireless_args.ch = arg_int0("c", "ch", "<1-13>", "WiFi channel");
    wireless_args.peer = arg_str0("p", "peer", "<MAC>", "Peer MAC address (XX:XX:XX:XX:XX:XX)");
    wireless_args.end = arg_end(2);
    const esp_console_cmd_t wire_cmd = {
        .command = "set_wireless",
        .help = "Configure wireless params",
        .argtable = &wireless_args,
        .func = &do_set_wireless,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wire_cmd));
    
    printf("StepSense CLI Ready. Use 'help' to see commands.\n");
    
    // 6. Start REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    return ESP_OK;
}
