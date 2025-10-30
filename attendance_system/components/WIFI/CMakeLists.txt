idf_component_register(
    SRCS "wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES driver freertos esp_wifi esp_event nvs_flash wifi_provisioning esp_http_client esp-tls rc522 i2c GPIO_CONF
)
