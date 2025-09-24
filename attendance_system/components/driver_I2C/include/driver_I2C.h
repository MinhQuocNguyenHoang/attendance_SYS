#pragma once
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // I2C initialize
    void i2c_master_init(void);

    // send command
    void lcd_command(uint8_t cmd);

    // send position
    void lcd_set_cursor(uint8_t col, uint8_t row);

    // send string
    void lcd_send_string(const char *str);

    // lcd initialize
    void lcd_init();

    // clear display
    void lcd_clear();

    // i2c_scanner
    void i2c_scanner();

#ifdef __cplusplus
}
#endif