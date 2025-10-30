#include "wifi.h"
#include "driver_I2C.h"
#include "GPIO_conf.h"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "rc522_ID";

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

void wifi_provising_config(void)
{
    // GPIO config
    LED_config();
    SW_config();

    // Khởi tạo wifi
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    // Đăng ký các hàm xử lý sự kiện
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Khởi tạo WiFi stack
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Logic chính của Provisioning
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
        lcd_send_string("AP: PRO_BIU");

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
            gpio_set_level(LED, 1);
            vTaskDelay(pdMS_TO_TICKS(2000)); // Chờ 2s để hiển thị thông báo
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