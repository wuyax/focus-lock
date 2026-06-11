# FocusLock Agent Guide (AGENTS.md)

This file provides the necessary context for AI agents to develop, build, and debug the FocusLock project efficiently.

## 1. Development Environment
- **Framework**: ESP-IDF v6.0.1
- **Hardware**: ESP32-S3 (SDA: GPIO 1, SCL: GPIO 2, LED: GPIO 48, BOOT: GPIO 0)
- **Target**: `esp32s3`

## 2. Mandatory Commands
Before running any `idf.py` command, you **MUST** source the environment.
- **Activation Command**: `source ~/.espressif/v6.0.1/esp-idf/export.sh`
- **Build & Flash Example**: `source ~/.espressif/v6.0.1/esp-idf/export.sh && idf.py -p /dev/cu.usbmodem21113201 build flash monitor`

## 3. macOS Serial Port Guidelines
- **ALWAYS** use `/dev/cu.*` instead of `/dev/tty.*`.
- **Port Types & Roles**:
  - **Native USB Port** (Labeled "USB"):
    - Used for: **USB HID (Lock Screen)**, Web Configuration, and modern debugging.
    - Identification: Typically appears as `/dev/cu.usbmodemXXXX`.
    - **MANDATORY**: This port MUST be connected to the host computer for the lock screen feature to work.
  - **UART/Bridge Port** (Labeled "UART"):
    - Used for: Legacy flashing and stable log monitoring.
    - Identification: Typically appears as `/dev/cu.usbserial-XXXX`.

## 4. Operational Setup
- **Development**: You can flash via either port, but **Native USB** is required for HID functionality.
- **Testing**: When testing HID, ensure the Agent guides the user to use the **Native USB** port.

## 4. Architectural Patterns
- **Concurrency**: Based on FreeRTOS.
- **Communication**: Services (RGB, OLED, HID, Network) must listen to `status_queue` for state changes.
- **Config**: Settings are stored in NVS via `config_mgr`.
- **USB HID**: Uses TinyUSB component. If the console is lost after app start, it's due to USB pin takeover.

## 5. Troubleshooting
- **Boot Loop**: If the device keeps restarting after initializing TinyUSB, ensure the console is NOT set to `USB_CDC` if manual TinyUSB initialization is also performed without proper coordination.
- **Waiting for Download**: If stuck in download mode, ensure GPIO 0 is not pulled low and press RESET.
