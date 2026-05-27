idf_component_register(SRCS "main.c" "tof_settings.c" "hw_interaction.c" "espnow_rx.c" "usb_cdc_bridge.c" "data_aggregator.c" "status_manager.c" "tof_cli.c"
                    INCLUDE_DIRS "."
                    REQUIRES console esp_driver_usb_serial_jtag esp_hw_support nvs_flash esp_wifi esp_netif esp_timer esp_driver_uart esp_driver_gpio)
