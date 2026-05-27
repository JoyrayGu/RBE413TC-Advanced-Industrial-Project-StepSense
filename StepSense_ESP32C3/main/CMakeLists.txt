idf_component_register(SRCS "main.c"
                            "espnow_sm.c"
                            "hw_interaction.c"
                            "tof_uart_rx.c"
                            "usb_cdc_bridge.c"
                            "tof_cli.c"
                            "tof_settings.c"
                    INCLUDE_DIRS "."
                    REQUIRES console nvs_flash esp_wifi esp_netif esp_timer driver esp_driver_uart esp_driver_gpio esp_driver_usb_serial_jtag)
