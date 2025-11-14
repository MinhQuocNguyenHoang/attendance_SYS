#pragma once
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include <esp_log.h>

#ifdef __cplusplus
extern "C"
{
#endif
    // confi wifi (include provising)
    void wifi_provising_config(void);

#ifdef __cplusplus
}
#endif