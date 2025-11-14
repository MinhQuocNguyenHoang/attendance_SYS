#include "http.h"
#include "driver_I2C.h"

#define HTTP_BUFFER_SIZE 1024
#define UID_MAX_LEN RC522_PICC_UID_STR_BUFFER_SIZE_MAX

static const char *TAG_Script = "App script";
static const char *TAG = "rc522_ID";

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
            else if (strstr(rb.buf, "doi hom sau") != NULL)
            {
                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_send_string("da diem danh roi");
                lcd_set_cursor(0, 1);
                lcd_send_string("doi hom sau");
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
