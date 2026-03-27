#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "init.h"

// Function declarations
void resetPin(uint8_t pin);
void handleVolumeControl(uint8_t volume);
void setPixelColor(led_strip_handle_t led_strip, int pixel_index, const Color& color);
void setPixelRange(led_strip_handle_t led_strip, int pixel_index_start, int pixel_index_end, const Color& color);
void setLEDColorRange(int potValue, int ledIndex, int minVal, int maxVal, int bright);
void encTurn(int value);
int readEncoder();
int readAnalogValue(adc_oneshot_unit_handle_t adc_handle, adc_channel_t channel);
void printBatteryStats();
void updateChargingStatus(bool charging);
void updateBatteryLevel(uint8_t level);
void checkSleepMode(int64_t& lastTimeCheck);
void vOff();
void vOn();
void keyPressed(char key);
void update_buttonset_label();
void highlight_button(char key);
void clear_button_highlight(char key);
#endif // FUNCTIONS_H