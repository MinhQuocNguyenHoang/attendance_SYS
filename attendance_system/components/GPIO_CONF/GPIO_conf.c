#include "GPIO_conf.h"

void LED_config(void)
{
    // khởi tạo led
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0, // có pull-up sẵn
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_conf);
}

void SW_config(void)
{
    // khởi tạo sw reset wifi
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RESET_WIFI),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1, // có pull-up sẵn
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}