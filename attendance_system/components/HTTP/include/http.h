#pragma once
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include <esp_log.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

typedef struct
{
    char *buf;
    size_t len;
} resp_buf_t;

#ifdef __cplusplus
extern "C"
{
#endif
    // send data
    void send_data_to_sheet(char *uid);

    // remove spaces
    void remove_spaces(char *src);
#ifdef __cplusplus
}
#endif
