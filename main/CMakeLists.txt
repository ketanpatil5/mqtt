idf_component_register(SRCS "app_main.c" "uart.c"
                    PRIV_REQUIRES mqtt esp_partition nvs_flash esp_netif app_update
		            REQUIRES esp_driver_uart esp_driver_gpio
                    INCLUDE_DIRS ".")
