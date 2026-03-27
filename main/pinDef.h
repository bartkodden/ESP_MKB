#define POT_1    39 // Analog in Potmeter
#define POT_2    36 // Analog in Potmeter
#define MINT      0 // Digital in, TCA8418 interrupt
#define MRES     10 // Digital out, TCA8418 reset
#define LEDS     GPIO_NUM_5 // Digital out, WS2812B data line
#define ENC_A    GPIO_NUM_37 // Digital in, encoder A
#define ENC_B    GPIO_NUM_38 // Digital in, encoder B
#define CHRG     35 // Digital in, MCP73871 Charging led output
#define PWR      32 // Plugged in state of MCP73871
#define DONE     34 // Charging Done (MCP73871)
#define BATMEAS  33 // Digital IO BQ27441 GPOUT
#define SLP_IO    4 // Digital out, shutdown power hungry IO

#define I2C_SDA  22 // i2c data
#define I2C_SCL  21 // i2c clock

#define SPI_SDA  13 // SPI MOSI
#define SPI_SCK  14 // SPI Clock
#define SPI_DC   27 // SPI DC
#define SPI_BL   26 // SPI Backlight
#define SPI_CS   15 // SPI Chip select

#define I2S_WS   18 // i2s word select
#define I2S_BCLK 23 // i2s base clock
#define I2S_DATA 19 // i2s data

/* OLD
#define POT_1     2 // Analog in Potmeter
#define POT_2     3 // Analog in Potmeter
#define MINT      6 // Digital in, TCA8418 interrupt
#define MRES      7 // Digital out, TCA8418 reset
#define LEDS      8 // Digital out, WS2812B data line
#define ENC_A    16 // Digital in, encoder A
#define ENC_B    17 // Digital in, encoder B
#define CHRG     19 // Digital in, MCP73871 Charging led output
#define BATMEAS  20 // Digital in

#define I2C_SDA  14 // i2c data
#define I2C_SCL  15 // i2c clock

#define I2S_WS   21 // i2s word select
#define I2S_BCLK 22 // i2s base clock
#define I2S_DATA 23 // i2s data

#define SPI_SDA   0 // SPI MOSI
#define SPI_SCK   1 // SPI Clock
#define SPI_DC    4 // SPI DC
#define SPI_BL    5 // SPI Backlight
#define SPI_CS   18 // SPI Chip select
*/
