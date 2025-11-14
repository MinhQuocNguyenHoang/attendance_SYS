#pragma once
#include "driver/gpio.h"
#include <esp_log.h>

#define LED 33
#define RESET_WIFI 16

#ifdef __cplusplus
extern "C"
{
#endif

    // LED config
    void LED_config(void);

    // SW config
    void SW_config(void);
#ifdef __cplusplus
}
#endif