# ESP_MKB
ESP32 driven macro keyboard

# Overview
This is a project born out of personal necessity to have a dedicated programmable macro keyboard on my desk which I can control easily and bind functions I use a lot to buttons of my own choice.
I have made a similar project ~5 years ago which featured an arduino pro micro, 8 buttons, encoder, rgb leds and oled screen.
This project aims to solve my main problem with that board: the wire going from the arduino to my PC.

By making this new project use an ESP32-C6 I will try to add a lot more functionality to the systems than was previously possible.

# Hardware
The main SOC is an ESP32-C6-Mini-1-N4. By using this specific ESP32 I will not only get WIFI+BT but also gain the option to later on include matter support and other cool stuff.
The hardware components I have included in this first revision are the following:
- ESP32-C6-Mini-1-N4
- Reset and boot mode select buttons
- Multiple display options:
  - SPI display option (<a href='https://aliexpress.com/item/1005006857426510.html'>1.69" 280x240 standard</a>)
  - UART display option (<a href='aliexpress.com/item/1005006862867338.html'>standard 1.3" I2C oled</a>)
- USB-C
- TCA8418 button matrix IC
- 8x Kailh hot swappable cherry style sockets for cherry MX style switches
- 4x simple push buttons 
- 2x short analog potmeters 
- 1x encoder
- 18x RGB LED (XL-1615RGBC-WS2812B on top side / 5050 WS2812B on bottom)
- BMS system consisting of:
  - MCP73871 Charge controller
  - BQ27441 Fuel gauge
  - single 18650 battery holder
 
# IO list
IO00 = SPI_SDA
IO01 = SPI_SCK
IO02 = POT_1
IO03 = POT_2
IO04 = SPI_DC
IO05 = SI_RES
IO06 = TCA8418 INTERRUPT
IO07 = TCA8418 MATRIX RESET
IO08 = LED DATA
IO09 = SPI_BL (+BOOT MODE)
IO12 = UD-
IO13 = UD+
IO14 = SDA
IO15 = SCL
IO16 = ENCODER A
IO17 = ENCODER B
IO18 = SPI_CS
IO19 = CHARGING (MCP73871)
IO20 = BATTERY MEASURE (BQ27441)
IO21 = MIC_WS
IO22 = MIC_BCLK
IO23 = MIC_DATA
