#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "time.h"
#include "driver_I2C.h"
#include "wifi.h"
#include "http.h"

static QueueHandle_t rfid_queue = NULL;
static const char *TAG_SYNC = "sync";
static const char *TAG = "rc522_ID";

#define RC522_SPI_BUS_GPIO_MISO (19)
#define RC522_SPI_BUS_GPIO_MOSI (23)
#define RC522_SPI_BUS_GPIO_SCLK (18)
#define RC522_SPI_SCANNER_GPIO_SDA (5)
#define RC522_SCANNER_GPIO_RST (-1) // soft-reset

#define UID_MAX_LEN RC522_PICC_UID_STR_BUFFER_SIZE_MAX

static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};
static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE)
    {
        char uid_str[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];
        esp_err_t ret = rc522_picc_uid_to_str(&picc->uid, uid_str, sizeof(uid_str));

        if (ret == ESP_OK)
        {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_send_string(uid_str);
            lcd_set_cursor(0, 1);
            lcd_send_string("dang gui");
            char uid_copy[UID_MAX_LEN];
            strlcpy(uid_copy, uid_str, sizeof(uid_copy));
            xQueueSend(rfid_queue, uid_copy, portMAX_DELAY);
        }
        else
        {
            ESP_LOGW(TAG, "Chuyển UID sang chuỗi thất bại");
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE)
    {
        ESP_LOGI(TAG, "Card has been removed");
    }
}

void rfid_scanner_task(void *parameters)
{
    // Khởi tạo rc522
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };
    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    lcd_set_cursor(0, 0);
    lcd_send_string("Moi ban quet the");
    rc522_start(scanner);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void http_post_task(void *parameters)
{
    char uid_buffer[UID_MAX_LEN];
    while (1)
    {
        if (xQueueReceive(rfid_queue, uid_buffer, portMAX_DELAY) == pdPASS)
        {
            rc522_pause(scanner);
            uid_buffer[UID_MAX_LEN - 1] = '\0';
            send_data_to_sheet(uid_buffer);
            rc522_start(scanner);
        }
    }
}

void app_main()
{
    rfid_queue = xQueueCreate(5, UID_MAX_LEN);
    if (rfid_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create RFID queue");
    }

    // khởi tạo LCD
    i2c_master_init();
    i2c_scanner();
    lcd_init();

    // khởi tạo wifi
    wifi_provising_config();

    // chạy task
    xTaskCreate(&http_post_task, "http", 4096, NULL, 4, NULL);
    xTaskCreate(&rfid_scanner_task, "rfid", 8000, NULL, 5, NULL);
}