# LoRa Chat (ESP32 + SX1278 + OLED + Rotary Encoder + Keypad)

A dual-device LoRa chat prototype built for college-level embedded systems work.

This project runs on ESP32 with SX1278 LoRa, SH1106 OLED, rotary encoder, and 4x4 keypad. It also includes a phone bridge (WiFi AP web UI + optional Bluetooth serial bridge) for remote control and messaging.

## Highlights

- Reliable LoRa messaging with ACK + retry flow
- Live packet-loss tracking and RF diagnostics (RSSI/SNR)
- OLED multi-page UI with keypad/encoder navigation
- Phone web UI for send/receive and remote controls
- Bridge modes:
  - WiFi AP + Web dashboard
  - Bluetooth serial text bridge
- Device-unique SSID/password/Bluetooth name/token derived from chip MAC
- Inbox/history on OLED and web
- Frequency tuning and preset-style workflow
- Config persistence across reboot (frequency, mode, quick state)

## Practical Range

Range depends on antenna quality, height, environment, and RF noise.

- Urban area (dense buildings/interference): up to about 5 km
- Open field / clear line-of-sight: about 10-15 km

These are practical prototype targets, not guaranteed limits.

## Hardware

- ESP32 development board
- SX1278 LoRa module (433 MHz)
- SH1106 128x64 OLED (I2C)
- Rotary encoder with push switch
- 4x4 matrix keypad
- 3.3V power source and proper grounding

See `hardware/` for schematic assets and source files.

## Firmware File

- Main sketch: `lora.ino`
- Partition file: `partitions.csv`

## Build and Flash

1. Install Arduino IDE (or VS Code + Arduino extension).
2. Install ESP32 board package.
3. Install required libraries:
   - LoRa
   - U8g2
   - Keypad
4. Open `lora.ino`.
5. Select ESP32 board/port.
6. Ensure partition scheme matches your profile needs.
7. Upload the same firmware to both devices.

## Quick Start Tutorial

### 1) Power and boot

- Power both nodes.
- OLED should show boot then home page.

### 2) Set radio frequency

- Open `RADIO` from home.
- Use encoder or keypad to tune.
- Press `#` to apply.
- Make sure both devices use the same frequency.

### 3) Send from device UI

- Open `CHAT`.
- Compose using picker/keypad.
- Press `#` to send.
- Check ACK/RX rows for delivery state.

### 4) Send from phone (WiFi bridge)

- On device open `BRIDGE`, select WiFi mode, apply.
- Read SSID/PASS on OLED bridge page.
- Connect phone to ESP hotspot.
- Open `http://192.168.4.1`.
- Send text from the web form.

### 5) Optional Bluetooth bridge

- In `BRIDGE`, switch to Bluetooth mode and apply.
- Pair phone to device Bluetooth name.
- Use a Bluetooth serial terminal app and send text lines.

### 6) Review inbox/history

- Open `INBOX` page on OLED to browse received messages.
- Use web UI inbox panel for phone-side history.

## Controls Summary

### Keypad

- `A`: Home
- `#`: Send/Confirm/Apply (context-dependent)
- `*`: Backspace (long press clears buffer)
- `D`: Context action (template/system/preset/clear)

### Encoder

- Rotate: Navigate/adjust by page (`HOME`, `CHAT`, `QUICK`, `RADIO`, `INBOX`, `BRIDGE`)
- Press: Select/append/send/apply by page

## Web UI Features

- Real-time status panel
- Send over LoRa
- Remote control actions (page, mode, freq, ping, maintenance)
- Dark/Light theme toggle with local persistence

## Project Structure

- `lora.ino` - firmware
- `partitions.csv` - ESP32 partitions
- `hardware/` - EasyEDA schematic source/PDF/PNG
- `docs/regulatory/india/` - India regulatory references and declaration
- `.gitignore` - keeps generated files out of git

## Regulatory Note

This is an educational prototype repository.

For India-specific references and declaration context, see:
- `docs/regulatory/india/README.md`

## Troubleshooting

- No LoRa init: verify wiring, SPI pins, power stability.
- No phone page: confirm WiFi bridge mode and hotspot connection.
- Pair but no BT messages: use a Bluetooth serial terminal app, send newline-terminated text.
- High loss: improve antenna placement, reduce obstructions, match frequency on both nodes.

