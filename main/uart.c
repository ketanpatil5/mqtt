/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (1)
#define ECHO_TEST_RXD (3)
#define ECHO_TEST_RTS (22)
#define ECHO_TEST_CTS (21)

#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_BAUD_RATE     (19200)
#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)

static const char *TAG = "UART TEST";

#define BUF_SIZE (1024)

typedef struct {
    char *KP;
    int len;
} TaskParams;


static void echo_task(void *pvParameters)
{
    TaskParams *params = (TaskParams *)pvParameters;
    char *KP = params->KP;
    int len = params->len;
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    
    // Read data from the UART
    // int len = uart_read_bytes(ECHO_UART_PORT_NUM, KP, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
    // Write data back to the UART
    uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) KP, len);
    if (len) {
        data[len] = '\0';
        ESP_LOGI(TAG, "Recv str: %s", (char *) KP);
        
    }
}

void uart(char* KP,int len)
{
    TaskParams params = { .KP = KP, .len = len };
    // xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, (void *)&params, 10, NULL);
    echo_task((void *)&params);
}
