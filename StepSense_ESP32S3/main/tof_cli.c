#include "sdkconfig.h"

// Force override any sdkconfig UART defaults to use the Type-C JTAG Console
#undef CONFIG_ESP_CONSOLE_UART_DEFAULT
#define CONFIG_ESP_CONSOLE_UART_DEFAULT 0
#undef CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#define CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG 1

#include "tof_cli.h"
#include "tof_settings.h"
#include "tof_protocol.h"
#include "nvs_flash.h"
#include "esp_console.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"

#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "espnow_rx.h"

// static const char *TAG = "TOF_CLI"; // Unused

// --- Command Handlers ---

static int do_show_peers(int argc, char **argv) {
    uint8_t macs[2][6];
    int count = espnow_rx_get_peers(macs);
    printf("\n--- Paired Foot Units (%d) ---\n", count);
    for (int i = 0; i < count; i++) {
        printf("[%d] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
               i, macs[i][0], macs[i][1], macs[i][2], macs[i][3], macs[i][4], macs[i][5]);
    }
    printf("----------------------------\n\n");
    return 0;
}

static int do_show_cfg(int argc, char **argv) {
    TofSettings_t *cfg = tof_settings_get();
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    printf("\n--- StepSense S3 Bridge Configuration ---\n");
    printf("Local MAC:   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("Left  Peer:  %02X:%02X:%02X:%02X:%02X:%02X\n", 
           cfg->peer_mac_l[0], cfg->peer_mac_l[1], cfg->peer_mac_l[2], cfg->peer_mac_l[3], cfg->peer_mac_l[4], cfg->peer_mac_l[5]);
    printf("Right Peer:  %02X:%02X:%02X:%02X:%02X:%02X\n", 
           cfg->peer_mac_r[0], cfg->peer_mac_r[1], cfg->peer_mac_r[2], cfg->peer_mac_r[3], cfg->peer_mac_r[4], cfg->peer_mac_r[5]);
    printf("Mirror:      %s\n", cfg->mirror_enabled ? "ON (Streaming to PC)" : "OFF (CLI Mode)");
    printf("WiFi Ch:     %d\n", cfg->wifi_ch);
    printf("----------------------------------------\n\n");
    return 0;
}

// --- MIRRORING ---
static int do_set_mirror(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: set_mirror <ON/OFF>\n");
        return 1;
    }
    TofSettings_t *cfg = tof_settings_get();
    if (strcasecmp(argv[1], "ON") == 0) cfg->mirror_enabled = 1;
    else if (strcasecmp(argv[1], "OFF") == 0) cfg->mirror_enabled = 0;
    else {
        printf("Invalid option. Use ON or OFF.\n");
        return 1;
    }
    tof_settings_save();
    printf("Mirror %s.\n", cfg->mirror_enabled ? "ENABLED" : "DISABLED");
    return 0;
}

static int do_restart(int argc, char **argv) {
    printf("Restarting system...\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
    return 0;
}

// --- Initialization ---

esp_err_t tof_cli_init(void) {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "StepSense-S3> ";
    repl_config.max_history_len = 10;

    // Initialize REPL based on hardware (Forced to USB-Serial-JTAG)
#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    printf("\n[CLI] S3 Configured via USB-Serial-JTAG (Type-C)\n");
#else
    // Fallback just in case macro logic fails
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    hw_config.baud_rate = 115200;
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
    printf("\n[CLI] S3 Configured via UART0 (115200 bps)\n");
#endif

    // Register Commands
    const esp_console_cmd_t show_cfg_cmd = {
        .command = "show_cfg",
        .help = "Show current bridge configuration",
        .func = &do_show_cfg,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&show_cfg_cmd));

    const esp_console_cmd_t mirror_cmd = {
        .command = "set_mirror",
        .help = "Enable/Disable PC data streaming",
        .hint = "<ON/OFF>",
        .func = &do_set_mirror,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&mirror_cmd));

    // Register show_peers
    const esp_console_cmd_t peers_cmd = {
        .command = "show_peers",
        .help = "List currently paired Foot units (C3)",
        .func = &do_show_peers,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&peers_cmd));

    // Register restart
    const esp_console_cmd_t restart_cmd = {
        .command = "restart",
        .help = "Restart the system",
        .func = &do_restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&restart_cmd));

    printf("StepSense S3 CLI Ready. Use 'help' to see commands.\n");

    return esp_console_start_repl(repl);
}
