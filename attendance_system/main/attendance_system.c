#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "driver/i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_sntp.h"
#include "time.h"
#include "esp_crt_bundle.h"
#include "freertos/queue.h"
#include "driver_I2C.h"

static QueueHandle_t rfid_queue = NULL;
static const char *TAG_Script = "App script";
static const char *TAG_SYNC = "sync";

static const char *TAG = "rc522_ID";

#define RC522_SPI_BUS_GPIO_MISO (19)
#define RC522_SPI_BUS_GPIO_MOSI (23)
#define RC522_SPI_BUS_GPIO_SCLK (18)
#define RC522_SPI_SCANNER_GPIO_SDA (5)
#define RC522_SCANNER_GPIO_RST (-1) // soft-reset

#define RESET_WIFI 25
#define UID_MAX_LEN RC522_PICC_UID_STR_BUFFER_SIZE_MAX
#define HTTP_BUFFER_SIZE 1024
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

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
void send_data_to_sheet(char *uid);

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
typedef struct
{
    char *buf;
    size_t len;
} resp_buf_t;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    resp_buf_t *rb = (resp_buf_t *)evt->user_data;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0)
        {
            // Mở rộng buffer: +1 để thêm '\0'
            char *p = realloc(rb->buf, rb->len + evt->data_len + 1);
            if (!p)
            {
                return ESP_FAIL; // hết RAM
            }
            rb->buf = p;
            memcpy(rb->buf + rb->len, evt->data, evt->data_len);
            rb->len += evt->data_len;
            rb->buf[rb->len] = '\0'; // ensure null-terminated
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

void remove_spaces(char *src)
{
    char *dst = src;
    while (*src)
    {
        if (*src != ' ')
        {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

void send_data_to_sheet(char *uid)
{
    char uid_clean[UID_MAX_LEN];
    strlcpy(uid_clean, uid, sizeof(uid_clean));
    remove_spaces(uid_clean);

    char url_with_param[300];
    snprintf(url_with_param, sizeof(url_with_param),
             "https://script.google.com/macros/s/AKfycbzX0h1RdpJCUkjpPu72SeEdHWKoVSEVjaw9e8-2GItjcO4N40-ktvV7Pziw3chv5Qzs/exec?UID=%s",
             uid_clean);

    resp_buf_t rb = {.buf = NULL, .len = 0};

    esp_http_client_config_t config = {
        .url = url_with_param,
        .method = HTTP_METHOD_GET,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler,
        .user_data = &rb,
        .is_async = false,
        .timeout_ms = 15000,
        .disable_auto_redirect = false,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(TAG_Script, "URL dùng trong ESP32: %s", config.url);
    esp_err_t err = esp_http_client_perform(client);
    // int status_code = esp_http_client_get_status_code(client);
    // ESP_LOGI(TAG_Script, "HTTP status: %d", status_code);
    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG_Script, "HTTP status = %d, content_length = %d", status_code, content_length);
        if (rb.buf)
        {
            ESP_LOGI(TAG_Script, "%s", rb.buf);
            if (strstr(rb.buf, "Da dang ki") != NULL)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string(uid);
                lcd_set_cursor(0, 1);
                lcd_send_string("da dang ki");
                vTaskDelay(pdMS_TO_TICKS(2000));
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string("Moi ban quet the");
            }
            else if (strstr(rb.buf, "diem danh vao") != NULL)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string(uid);
                lcd_set_cursor(0, 1);
                lcd_send_string("diem danh vao");
                vTaskDelay(pdMS_TO_TICKS(2000));
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string("Moi ban quet the");
            }
            else if (strstr(rb.buf, "diem danh ra") != NULL)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string(uid);
                lcd_set_cursor(0, 1);
                lcd_send_string("diem danh ra");
                vTaskDelay(pdMS_TO_TICKS(2000));
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string("Moi ban quet the");
            }
            else
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string("Phan hoi sai");
            }
            free(rb.buf);
        }
    }
    else
    {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_send_string("Failed");
        ESP_LOGI(TAG, "Failed");
    }
    esp_http_client_cleanup(client);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_send_string("Moi ban quet the");
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
    // khởi tạo boot
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RESET_WIFI),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1, // có pull-up sẵn
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    // khởi tạo LCD
    i2c_master_init();
    i2c_scanner();
    lcd_init();
    // Khởi tạo wifi
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    // 3. Đăng ký các hàm xử lý sự kiện
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // 4. Khởi tạo WiFi stack
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 5. Logic chính của Provisioning
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    if (!provisioned)
    {
        // --- CHƯA CÓ CẤU HÌNH WIFI ---
        ESP_LOGI(TAG, "Starting provisioning");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_send_string("Cau hinh WiFi...");
        lcd_set_cursor(0, 1);
        lcd_send_string("AP: PROV_ESP32");

        // SSID = PROV_ESP32, PASSWORD = 12345678
        const char *service_name = "PRO_BIU";
        const char *service_key = "12345678";

        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, NULL, service_name, service_key));
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    }
    else
    {
        // --- ĐÃ CÓ CẤU HÌNH WIFI ---
        ESP_LOGI(TAG, "Already provisioned, starting station mode");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_send_string("Da co cau hinh,");
        lcd_set_cursor(0, 1);
        lcd_send_string("dang ket noi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        wifi_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        // 6. Chờ đến khi kết nối WiFi thành công
        EventBits_t bit = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, false, pdMS_TO_TICKS(30000));
        // 7. Sau khi đã có WiFi, khởi tạo các tác vụ còn lại
        if (bit & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Connected!");
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_send_string("Thanh cong");
            vTaskDelay(pdMS_TO_TICKS(2000)); // Chờ 2s để hiển thị thông báo
            xTaskCreate(&http_post_task, "http", 4096, NULL, 4, NULL);
            xTaskCreate(&rfid_scanner_task, "rfid", 8000, NULL, 5, NULL);
        }
        else
        {
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_send_string("Connect Failed!");
            lcd_set_cursor(0, 1);
            lcd_send_string("Press to reset");
            while (1)
            {
                if (gpio_get_level(RESET_WIFI) == 0)
                {
                    lcd_set_cursor(0, 0);
                    lcd_send_string("Reset Wi-Fi");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    nvs_flash_erase(); // Xóa Wi-Fi đã lưu
                    esp_restart();
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}