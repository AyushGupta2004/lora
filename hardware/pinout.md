# Pinout

## LoRa SX1278 (SPI)

| ESP32 Pin | LoRa Pin | Signal |
|---|---|---|
| GPIO18 | SCK | SPI clock |
| GPIO19 | MISO | SPI MISO |
| GPIO23 | MOSI | SPI MOSI |
| GPIO5  | NSS  | Chip select |
| GPIO14 | RST  | Reset |
| GPIO2  | DIO0 | Interrupt |
| 3V3    | VCC  | Power |
| GND    | GND  | Ground |

## OLED SH1106 (I2C)

| ESP32 Pin | OLED Pin | Signal |
|---|---|---|
| GPIO21 | SDA | I2C data |
| GPIO22 | SCL | I2C clock |
| 3V3    | VCC | Power |
| GND    | GND | Ground |

## Rotary Encoder

| ESP32 Pin | Encoder Pin | Signal |
|---|---|---|
| GPIO34 | CLK | Quadrature A |
| GPIO35 | DT  | Quadrature B |
| GPIO27 | SW  | Push button |
| 3V3    | +   | Power |
| GND    | GND | Ground |

Note: GPIO34/GPIO35 are input-only and do not provide internal pull-ups. Use encoder module pull-ups or external pull-up resistors.

## 4x4 Keypad

| ESP32 Pin | Keypad Pin |
|---|---|
| GPIO13 | R1 |
| GPIO12 | R2 |
| GPIO4  | R3 |
| GPIO15 | R4 |
| GPIO32 | C1 |
| GPIO33 | C2 |
| GPIO25 | C3 |
| GPIO26 | C4 |
