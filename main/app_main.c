#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include "driver/gpio.h"

#define UART_NUM UART_NUM_1
#define TXD_PIN (17) // Change to your TX pin
#define RXD_PIN (16) // Change to your RX pin
#define BAUD_RATE 115200      // Typical for SIMCom modules 
#define RESPONSE_BUF_SIZE 1024
#define GPIO_PIN 32

void gpio_init() {
  gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_PIN,  // Configure GPIO32
        .mode = GPIO_MODE_OUTPUT,         // Set as output mode
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE    // No interrupts
    };
    gpio_config(&io_conf);
}

void generate_negative_pulse(uint32_t pin, uint32_t pulse_width_ms) {
    // Set the pin HIGH
    gpio_set_level(pin, 1);
    // vTaskDelay(pdMS_TO_TICKS(10));  // Ensure the pin is stable before generating the pulse

    // Set the pin LOW (start of negative pulse)
    ESP_LOGI("GPIO", "Generating negative pulse...");
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(pulse_width_ms));  // Hold the pin LOW for the pulse width

    // Set the pin HIGH again (end of pulse)
    gpio_set_level(pin, 1);
    ESP_LOGI("GPIO", "Pulse completed.");
}

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024*2, 0, 0, NULL, 0);
}


esp_err_t send_at_command(const char *cmd, char *response, int timeout_ms) {
    uart_write_bytes(UART_NUM, cmd, strlen(cmd));
    uart_write_bytes(UART_NUM, "\r\n", 2);

    // Wait for response
    int len = uart_read_bytes(UART_NUM, (uint8_t *)response, RESPONSE_BUF_SIZE - 1, pdMS_TO_TICKS(timeout_ms));
    if (len > 0) {
        response[len] = '\0'; // Null-terminate the response
        ESP_LOGI("UART", "response: %s", response);
        return ESP_OK;
    }
    ESP_LOGE("UART", "UART failed!");
    
    return ESP_FAIL;
}


void mqtt_init(const char *broker, const char *username, const char *password) {
    char response[RESPONSE_BUF_SIZE];
    generate_negative_pulse(GPIO_PIN, 10);
    send_at_command("AT\r\n",response,500);
    send_at_command("AT+CFUN=1\r\n",response,500);
    send_at_command("AT+CREG=2\r\n",response,500);
    send_at_command("AT+CEREG=2\r\n",response,500);
    // send_at_command("AT+COPS?\r\n",response,1000);
    send_at_command("AT+CGDCONT=1,\"IP\",\"airtelgprs.com\"\r\n",response,500);
    send_at_command("AT+CGACT=0,1\r\n",response,500);     
    send_at_command("AT+CNTP=\"asia.pool.ntp.org\",0\r\n",response,500);
    send_at_command("AT+CNTP\r\n",response,500);  
    // send_at_command("AT+CCLK?\r\n",response,1000);
    // send_at_command("AT+IPADDR\r\n",response,1000);
    send_at_command("AT+CGATT=1\r\n",response,500);
    send_at_command("AT+CGPADDR=1\r\n",response,500);
    // send_at_command("AT+CGACT?\r\n",response,1000);
    // send_at_command("AT+CEREG?\r\n",response,1000);
    send_at_command("AT+NETOPEN\r\n",response,500);
    // send_at_command("AT+NETOPEN?\r\n",response,1000);
    
    // send_at_command("AT+CSSLCFG=\"sslversion\",0,4\r\n",response,1000);
    send_at_command("AT+CSSLCFG=\"authmode\",0,1\r\n",response,500);
    send_at_command("AT+CSSLCFG=\"cacert\",0,\"ca_cert.pem\"\r\n",response,500);
    // send_at_command("AT+CSSLCFG=\"clientcert\",0,\"client_cert.pem\"\r\n",response,1000);
    // send_at_command("AT+CSSLCFG=\"clientkey\",0,\"client_key.pem\"\r\n",response,1000);
    // send_at_command("AT+CSSLCFG=\"password\",0,\"password.pem\"\r\n",response,1000);
    send_at_command("AT+CSSLCFG=\"enableSNI\",0,1\r\n",response,500);

    send_at_command("AT+CMQTTSTART\r\n", response, 500);
    send_at_command("AT+CCHSET=1,1\r\n", response, 500);
    send_at_command("AT+CCHMODE=1\r\n", response, 500);
    
    send_at_command("AT+CCHSTART\r\n", response, 500);
    send_at_command("AT+CCHADDR\r\n", response, 500);
    

    // send_at_command("AT+CCHOPEN\r\n", response, 500);
    send_at_command("AT+CCHCFG=\"sendtimeout\",0,120\r\n", response, 500);
    send_at_command("AT+CCHCFG=\"sslctx\",0,1\r\n", response, 500);
    // send_at_command("AT+CCERTMOVE=\"ca_cert.pem\"\r\n", response, 500);
    
    send_at_command("AT+CMQTTSSLCFG=0,1\r\n", response, 500);    
    send_at_command("AT+CSSLCFG=0\r\n",response,500);
    
    
    //Set ID
    char mqtt_id[128];
    char mqtt_broker_copy[128];
    strncpy(mqtt_broker_copy, broker, sizeof(mqtt_broker_copy));
    mqtt_broker_copy[sizeof(mqtt_broker_copy) - 1] = '\0';  // Ensure null termination
    
    char *token = strtok(mqtt_broker_copy, "."); 
    if (token != NULL) {
        snprintf(mqtt_id, sizeof(mqtt_id), "AT+CMQTTACCQ=0,\"%s\",1\r\n", token);
        send_at_command(mqtt_id, response, 1000);
        
    } else {
        ESP_LOGI("MQTT", "Error parsing broker ID");
    }
         
    // send_at_command("AT+CMQTTCONNECT?\r\n",response,100);
    
    // send_at_command("AT+CCERTLIST", response, 1000);
    // send_at_command("AT+CCERTDELE=\"ca_cert.pem\"", response, 1000);

    for(int i=0;i<3;i++){
        char mqtt_conn[256];
    snprintf(mqtt_conn, sizeof(mqtt_conn), "AT+CMQTTCONNECT=0,\"tcp://%s\",60,1,\"%s\",\"%s\"",
             broker, username, password);
    send_at_command(mqtt_conn, response, 2500);    
    }


    // ESP_LOGI("MQTT", "MQTT connection response: %s", response);

}

void mqtt_publish(const char *topic, const char *data) {
    char response[RESPONSE_BUF_SIZE];
    // Set topic
    char mqtt_topic[128];
    int len;
    len=strlen(topic);
    snprintf(mqtt_topic, sizeof(mqtt_topic), "AT+CMQTTTOPIC=0,%d", len);
    send_at_command(mqtt_topic, response, 1000);
    send_at_command(topic,response,1000);
    
    // Send payload
    char mqtt_payload[512];
    len=strlen(data);
    snprintf(mqtt_payload, sizeof(mqtt_payload), "AT+CMQTTPAYLOAD=0,%d\r\n", len);
    send_at_command(mqtt_payload, response, 1000);
    send_at_command(data,response,1000);   

    // Publish Data
    send_at_command("AT+CMQTTPUB=0,0,60\r\n",response,500);

    // send_at_command("AT+CMQTTDISC=0,60\r\n", response, 2000);
    // send_at_command("AT+CMQTTSTOP\r\n",response,1000);


    ESP_LOGI("MQTT", "MQTT publish response: %s", response);
}

void mqtt_subscribe(const char *topic, const char *data) {
    char response[RESPONSE_BUF_SIZE];
    
    // Set topic
    char mqtt_topic[128];
    int len;
    len=strlen(topic);
    snprintf(mqtt_topic, sizeof(mqtt_topic), "AT+CMQTTSUBTOPIC=0,%d,0", len);
    send_at_command(mqtt_topic, response, 1000);
    send_at_command(topic,response,1000);
 
    
    // Send payload
    char mqtt_payload[512];
    len=strlen(data);
    snprintf(mqtt_payload, sizeof(mqtt_payload), "AT+CMQTTSUB=0,%d,0\r\n", len);
    send_at_command(mqtt_payload, response, 1000);
    send_at_command(data,response,1000);

    // Disconnect
    // len=strlen(topic);
    // snprintf(mqtt_topic, sizeof(mqtt_topic), "AT+CMQTTUNSUBTOPIC=0,%d", len);
    // send_at_command(mqtt_topic, response, 1000);
    // send_at_command(topic,response,1000);


    // send_at_command("AT+CMQTTDISC=0,60\r\n", response, 2000);
    // send_at_command("AT+CMQTTSTOP\r\n",response,1000);


    ESP_LOGI("MQTT", "MQTT publish response: %s", response);
}


void app_main() {
    uart_init();
    gpio_init();
    // Initialize MQTT
    const char *broker = "61f18205d9d64ad1b29f6e99d0679efc.s1.eu.hivemq.cloud:8883";
    const char *username = "demo1";
    const char *password = "Demo1234";
    mqtt_init(broker, username, password);

    // Publish data periodically
    // int i=0;
    // while (1) {
    //     char mqtt_payload[512];
  
    //     snprintf(mqtt_payload, sizeof(mqtt_payload), "{\"temperature\": 2500.6}", len);
        
        mqtt_publish("esp32/test", "{\"temperature\": 2500.6}");
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publish every 10 seconds
        // i++;
    // }
    // mqtt_subscribe("esp32/test", "{\"temperature\": 25515}");
}
