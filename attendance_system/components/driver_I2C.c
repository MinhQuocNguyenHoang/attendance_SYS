#include "driver_I2C.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

#define I2C_MASTER_SCL_IO (27)
#define I2C_MASTER_SDA_IO (26)
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ (100000)
#define LCD_COMMAND 0
#define LCD_DATA 1
#define LCD_ADDRESS 0x27
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_RS 0x01

static uint8_t lcd_adr = LCD_ADDRESS;
static const char *TAG_I2C = "I2C";

// hàm nội bộ
static void lcd_send_i2c(uint8_t data)
{
    i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDRESS, &data, 1, pdMS_TO_TICKS(1000));
}

static void lcd_strobe(uint8_t data)
{
    lcd_send_i2c(data | LCD_ENABLE | LCD_BACKLIGHT);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_send_i2c((data & ~LCD_ENABLE) | LCD_BACKLIGHT);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void lcd_send_4bit(uint8_t nibble, uint8_t rs_flag)
{
    uint8_t data = (nibble << 4); // D4~D7 nằm tại P4~P7
    if (rs_flag)
        data |= LCD_RS;
    data |= LCD_BACKLIGHT;
    lcd_strobe(data);
}

static void lcd_send_8bit(uint8_t byte, uint8_t rs_flag)
{
    lcd_send_4bit(byte >> 4, rs_flag);
    lcd_send_4bit(byte & 0x0F, rs_flag);
}

// hàm public
void i2c_scanner()
{
    ESP_LOGI(TAG_I2C, "Scanning I2C bus...");
    for (int addr = 1; addr < 127; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG_I2C, "I2C device found at address 0x%02X", addr);
        }
    }
}

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void lcd_command(uint8_t cmd)
{
    lcd_send_8bit(cmd, 0);
}

void lcd_send_data(uint8_t data)
{
    lcd_send_8bit(data, LCD_RS);
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_send_string(const char *str)
{
    while (*str)
    {
        lcd_send_data(*str++);
    }
}

void lcd_init()
{
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_send_4bit(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_4bit(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    lcd_send_4bit(0x03, 0);
    lcd_send_4bit(0x02, 0);

    lcd_command(0x28); // Function Set: 4-bit, 2 dòng, font 5x8
    lcd_command(0x0C); // Display Control: Bật màn hình, tắt con trỏ
    lcd_command(0x06); // Entry Mode Set: Tự động tăng con trỏ
    lcd_command(0x01); // Xóa màn hình
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_clear()
{
    lcd_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}