# Arduino Pasteurizer Controller

This project is an Arduino-based controller for managing a pasteurization process. It reads temperature data via analog inputs and communicates with a Python GUI over Wi-Fi. The controller supports both production and test/simulation modes.

## Features

- Wi-Fi communication using the Arduino Uno R4 WiFi
- Temperature monitoring from analog sensors
- Control commands for heating, cooling, and stopping the process
- Optional simulation mode for development/testing
- Designed to work with a Python/Tkinter GUI for live monitoring and control

## Requirements

- **Board:** Arduino Uno R4 WiFi
- **PlatformIO** with the following dependencies:
  - `WiFiNINA` library

## Setup Instructions

1. Clone this repository and open it with [PlatformIO](https://platformio.org/).
2. Create a `.env` file in the root of the project to specify which board/environment to build for. Example:

   ```env
   PIO_ENV=uno_r4_wifi
   ```
3. Create a `config.h` file in the `include/` directory (if it doesn’t exist yet).
4. Add your Wi-Fi credentials to `config.h` like so:

   ```cpp
   // config.h
   #ifndef CONFIG_H
   #define CONFIG_H

   #define WIFI_SSID "your_wifi_name"
   #define WIFI_PASS "your_wifi_password"

   #endif
   ```
