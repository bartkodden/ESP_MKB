#ifndef SETUPFUNCTIONS_H
#define SETUPFUNCTIONS_H

#include "esp_err.h"
#include "driver/gpio.h"
#include "led_strip.h"

esp_err_t i2c_master_init(void);
esp_err_t setupLeds(void);
esp_err_t resetLEDs(void);
esp_err_t setupPM(void);
esp_err_t setupDisplay(void);
esp_err_t setupBQ27441(void);
esp_err_t setupTCA8418(void);
esp_err_t setupADC(void);
esp_err_t setupMenu(void);
esp_err_t setupEncoder(gpio_num_t pin_a, gpio_num_t pin_b, int min, int max, 
                       bool wrap_around, void (*callback)(int));
esp_err_t setupBluetooth(void);

#endif // SETUPFUNCTIONS_H
