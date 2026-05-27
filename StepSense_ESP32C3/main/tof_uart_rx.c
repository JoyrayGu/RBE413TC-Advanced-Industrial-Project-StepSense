#include "tof_uart_rx.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define RX_BUF_SIZE 2048
#define UART_PORT UART_NUM_1

static const char *TAG = "UART_RX";
static tof_packet_cb_t packet_cb = NULL;

static void uart_rx_task(void *arg) {
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE);
    uint8_t packet_buffer[PACKET_TOTAL_LEN];
    int buf_len = 0;

    // Handshake: Send 'A'
    const char *handshake = "A";
    uart_write_bytes(UART_PORT, handshake, 1);
    ESP_LOGI(TAG, "Sent Handshake 'A' to STM32F401");

    while (1) {
        int rx_bytes = uart_read_bytes(UART_PORT, data, RX_BUF_SIZE, 10 / portTICK_PERIOD_MS);
        if (rx_bytes > 0) {
            for (int i = 0; i < rx_bytes; i++) {
                packet_buffer[buf_len++] = data[i];

                if (buf_len >= 2) {
                    if (packet_buffer[0] != 0xAA || packet_buffer[1] != 0x55) {
                        // Shift if sync word not found
                        memmove(packet_buffer, packet_buffer + 1, buf_len - 1);
                        buf_len--;
                    } else if (buf_len == PACKET_TOTAL_LEN) {
                        // Validate checksum
                        uint32_t calc_sum = 0;
                        for (int j = 0; j < 12; j++) calc_sum += packet_buffer[j];
                        for (int j = PACKET_HEADER_LEN; j < PACKET_TOTAL_LEN; j++) calc_sum += packet_buffer[j];
                        
                        calc_sum &= 0xFFFF;
                        uint16_t packet_sum = packet_buffer[12] | (packet_buffer[13] << 8);

                        if (calc_sum == packet_sum) {
                            if (packet_cb) {
                                packet_cb((const UartPacket_t *)packet_buffer);
                            }
                        } else {
                            ESP_LOGW(TAG, "Checksum failed! Calc: %04X, Pkt: %04X", (unsigned int)calc_sum, packet_sum);
                        }
                        
                        buf_len = 0; // Reset for next packet
                    }
                }
            }
        }
    }
    free(data);
}

void tof_uart_rx_init(int tx_pin, int rx_pin, uint32_t baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    ESP_LOGI(TAG, "UART1 initialized on TX:%d RX:%d @ %u baud", tx_pin, rx_pin, (unsigned int)baud_rate);
}

void tof_uart_rx_set_callback(tof_packet_cb_t cb) {
    packet_cb = cb;
}
