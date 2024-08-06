# Arduino code for ESP_MKB
Since I am most familiar with arduino, my first iteration of the software for ESP_MKB will be done in arduino IDE.
I understand ESP-IDF is more powerful or feature rich but I can't get it to work reliably.

# Goals for the arduino ESP_MKB software stack
Since I know the arduino IDE is more basic I have set the following goals for the first software iteration:
- All pins need to be addressed and communicated with
  - Analog and digital pins read and writable
  - Serial communications working (I2C, I2S, SPI)
- Clean code, seperate files for functions, initialization, pin definitions etc.
- Wifi communication enabled
- Basic preference saving enabled

Omissions
- No need for advanced functions:
  - No MQTT
  - No audio forwarding or handling (only function check)
