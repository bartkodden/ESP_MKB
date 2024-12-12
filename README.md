# ESP_MKB
ESP32 driven macro keyboard built for personal use but documented for the sake of open source development.

# Overview
This is a project born out of personal necessity to have a dedicated programmable macro keyboard on my desk which I can control easily and bind functions I use a lot to buttons of my own choice.
I have made a similar project ~5 years ago which featured an arduino pro micro, 8 buttons, encoder, rgb leds and oled screen.
This project aims to solve my main problem with that board: the wire going from the arduino to my PC.

By making this new project use an ESP32-C6 I will try to add a lot more functionality to the systems than was previously possible.
My intent for this project is the ability to support multiple use cases, either directly connected to a PC, or by having it connected to a network where it will be able to connect to home assistant.

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
  - MCP73871 Charge controller (Vout = Vbat (~3.7v) or Vout = Vusb (5.0v) when charging)
  - BQ27441 Fuel gauge
  - single 18650 battery holder
- Voltage regulation happens after MCP73871:
  - TPS63051 3.3v buck-boost regulator (Vin min = 2.5v - Vin max = 5.5v)
  - TPS613222 5.0v boost regulator (Vin min = 0.9v - Vin max = 5.5v)
 
# 3D preview
![3d preview](https://raw.githubusercontent.com/bartkodden/ESP_MKB/7820651de039026d2021146be1b5c34a28a80707/Hardware/Files/3D_PCB_top.png)

# IO list
<table>
  <tr>
    <th>IO PIN</th>
    <th>Function</th>
    <th>Detail</th>
  </tr>
  <tr><td>IO00</td><td>SPI SDA</td><td></td></tr>
  <tr><td>IO01</td><td>SPI SCK</td><td></td></tr>
  <tr><td>IO02</td><td>POT 1</td><td>Analog</td></tr>
  <tr><td>IO03</td><td>POT 2</td><td>Analog</td></tr>
  <tr><td>IO04</td><td>SPI DC</td><td></td></tr>
  <tr><td>IO05</td><td>SPI RES</td><td>On rev 0 board SPI screen needs this pin to be used as SPI_BL</td></tr>
  <tr><td>IO06</td><td>TCA8418 INTERRUPT</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO07</td><td>TCA8418 MATRIX RESET</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO08</td><td>LED DATA</td><td>Pullup 10K to 3v3, Mistake on Rev 0: Ldata needs pullup to 5V0. Cut trace on rev 0 to disable pullup</td></tr>
  <tr><td>IO09</td><td>SPI BL</td><td>Rev 0 mistake: Can not be used as SPI BL in combination with Boot mode on IO9</td></tr>
  <tr><td>IO12</td><td>UD-</td><td></td></tr>
  <tr><td>IO13</td><td>UD+</td><td></td></tr>
  <tr><td>IO14</td><td>I2C SDA</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO15</td><td>I2C SCL</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO16</td><td>ENCODER A</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO17</td><td>ENCODER B</td><td>Pullup 10K to 3v3</td></tr>
  <tr><td>IO18</td><td>SPI CS</td><td></td></tr>
  <tr><td>IO19</td><td>CHARGING Y/N</td><td>MCP73871 Pullup 10K to 3v3</td></tr>
  <tr><td>IO20</td><td>BATTERY MEASURE</td><td>BQ27441 Pullup 10K to 3v3</td></tr>
  <tr><td>IO21</td><td>I2S WS</td><td></td></tr>
  <tr><td>IO22</td><td>I2S BCLK</td><td></td></tr>
  <tr><td>IO23</td><td>I2S DATA</td><td></td></tr> 
</table>

# Schematic
ESP32C6, resistor networks, switches, communication buses and mounting hardware:
![ESP32 schematic](https://raw.githubusercontent.com/bartkodden/ESP_MKB/main/Hardware/Files/sch_esp32.png)

Power supply, battery management system and USB connection and voltage regulation
![Power schematic](https://raw.githubusercontent.com/bartkodden/ESP_MKB/main/Hardware/Files/sch_pwr.png)

Main IO
![IO schematic](https://raw.githubusercontent.com/bartkodden/ESP_MKB/main/Hardware/Files/sch_IO.png)
