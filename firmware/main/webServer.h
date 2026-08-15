#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t webserver_start(uint16_t port);
esp_err_t webserver_stop(void);

#ifdef __cplusplus
}
#endif