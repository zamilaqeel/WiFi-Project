# ESP32 WiFi Provisioning and LED Controller

This project is a web-based WiFi manager and RGB LED controller for the ESP32-S3-DevKitC-1 board. It allows you to configure WiFi credentials, control an RGB LED (color, brightness, on/off), and view system information via a responsive web interface with dark mode support.

## Features

- **WiFi Manager**: Scan, connect, and store WiFi credentials via a web portal.
- **Access Point Fallback**: Device always starts in AP+STA mode. If WiFi connection fails, the AP remains available for configuration.
- **RGB LED Control**: Change color, brightness, and toggle the LED on/off from the web UI.
- **Persistent Settings**: WiFi credentials and LED state are saved in non-volatile storage.
- **System Info**: View device status, IP addresses, uptime, and free memory.
- **Dark Mode**: Toggle between light and dark themes in the web UI.
- **Reset Credentials**: Long-press the boot button or use the web UI to clear WiFi credentials.

## Hardware

- **Board**: ESP32-S3-DevKitC-1
- **LED**: WS2812B (or compatible) RGB LED connected to pin 48

## Getting Started

### 1. Clone the Repository

```sh
git clone <your-repo-url>
cd <project-folder>
```

### 2. PlatformIO Setup

- Install [PlatformIO](https://platformio.org/) in VS Code.
- Connect your ESP32-S3-DevKitC-1 board.

### 3. `platformio.ini` Example

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
build_type = release

lib_deps =
    fastled/FastLED@^3.6.0
```

### 4. Build and Upload

- Open the project in VS Code.
- Click the PlatformIO "Upload" button or run:

```sh
pio run --target upload
```

### 5. Usage

- On first boot, connect to the `LED_CONFIG` WiFi AP (password: `12345678`).
- Open `http://192.168.4.1` in your browser.
- Use the web interface to scan for WiFi networks, enter credentials, and control the LED.
- After connecting to WiFi, the device will be accessible at its new IP address (shown in the UI).

## Web Interface

- **Home**: Status overview.
- **WiFi Setup**: Scan/connect to WiFi, clear credentials.
- **LED Control**: Change color, brightness, toggle on/off.
- **Info**: System and network information.

## Resetting WiFi Credentials

- Long-press the boot button (GPIO 0) for 3 seconds, or
- Use the "Clear Credentials" button in the web UI.
