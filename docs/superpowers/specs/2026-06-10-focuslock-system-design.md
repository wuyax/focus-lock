# FocusLock System Design Specification

## 1. Overview
This document outlines the software system design for FocusLock, an ESP32-S3-based hardware Pomodoro timer designed to enforce rest through physical intervention (USB HID screen locking). It incorporates various hardware peripherals including an OLED display (SSD1306), a DS3231 Hardware RTC, an active Buzzer, a physical Button, and a WS2812B RGB LED.

## 2. Hardware Pinout (ESP32-S3)
- **I2C Bus (Shared):** SDA -> GPIO 1, SCL -> GPIO 2. Used by both the SSD1306 OLED Display (0x3C) and DS3231 RTC (0x68).
- **Button:** GPIO 0 (Active low, internal pull-up).
- **Buzzer:** GPIO 3 (Active low, active buzzer module).
- **RGB LED (WS2812B):** GPIO 8 (Driven via RMT).
- **USB HID:** Native USB D+/D- pins (typically GPIO 19/20 on ESP32-S3).

## 3. System Architecture
The system employs a FreeRTOS multi-tasking architecture to decouple timekeeping, hardware control, and network services.

### 3.1 Core Tasks & Services
- **Pomodoro Engine (Task):** The central state machine. Manages the countdown, state transitions, and broadcasts state changes to other services via FreeRTOS Queues/Event Groups.
- **I2C Manager:** Centralized initialization for the shared I2C bus.
- **RTC Service (Task/Driver):** Communicates with the DS3231 to provide accurate real-time clock data for display and logging.
- **UI Service (Task):** Controls the OLED display and RGB LED animations.
- **Hardware Service (Task):** Manages Buzzer audio sequences and USB HID outputs.
- **Network Service (Task):** Manages the Wi-Fi AP and Web Server. Runs on-demand.

## 4. State Machine (Pomodoro Engine)

### 4.1 States
- `STATE_WORK`: Normal countdown (Default 45 mins).
- `STATE_WARNING`: Pre-rest warning phase (Default 30 secs).
- `STATE_REST`: Forced rest phase (Default 5 mins). Lock screen commands are dispatched.
- `STATE_PAUSE`: Temporary pause triggered by short press. Includes a 5-minute timeout; if not resumed, the current session is discarded.
- `STATE_ADMIN`: Indefinite pause. Triggered by a 5-second long press. **Activates Wi-Fi AP for Web UI access.**

### 4.2 Button Event Mapping (External Interrupt -> Timer)
- **Short Press:** `WORK` -> `PAUSE`, `PAUSE` -> `WORK`. Invalid in `REST`.
- **Double Click:** `WORK`/`WARNING` -> `REST` (Skip work), `REST` -> `WORK` (Skip rest).
- **Long Press (5s):** Toggles `ADMIN` mode.

## 5. Hardware & UI Services

### 5.1 OLED Display
- **Real-Time Clock:** Displays current time (HH:MM) fetched from the DS3231 in an unobtrusive location (e.g., top corner) across most states.
- **WORK/PAUSE/ADMIN:** Displays current state, MM:SS countdown, and a circular progress ring.
- **WARNING:** Displays large, blinking seconds countdown (e.g., "30", "29") to urge the user to save work.
- **REST:** Displays "REST", countdown, and progress ring.

### 5.2 RGB LED (WS2812B)
- `WORK`: Solid Green.
- `WARNING`: Breathing Yellow (managed by a software timer).
- `REST`: Solid Red.
- `PAUSE`/`ADMIN`: Solid Blue.

### 5.3 Buzzer
- Enter `WARNING`: 3 slow beeps (1s interval).
- Enter `REST` (Lock screen): 5 rapid beeps.
- Exit `REST`: 1 long beep.

### 5.4 USB HID (Lock Screen)
- Acts as a USB Keyboard.
- **Action:** Upon entering `REST`, sends a custom key combination.
- **Repetition:** If enabled, repeats the key combination every 10 seconds during `REST`.

## 6. Network & Configuration

### 6.1 Network Strategy
- **AP On-Demand:** Wi-Fi is strictly disabled during normal operation to save power and memory.
- **Trigger:** Wi-Fi AP (SSID: `FocusLock_Config`) is ONLY activated when entering `STATE_ADMIN`.
- **Captive Portal:** DNS hijacking redirects connected clients to the configuration page.
- **Shutdown:** Wi-Fi disables upon exiting `ADMIN` mode.

### 6.2 Web Configuration UI
A static HTML/JS single-page application embedded in the firmware. Communicates via RESTful APIs.

**Configuration Options:**
- **Pomodoro Parameters:** Work time, Rest time, Warning time.
- **Lock Screen Settings:** **Custom Key Combination** (Users input their specific shortcut, e.g., `Cmd+L`). OS presets can be offered as templates in the UI. Repeat lock toggle and interval.
- **Hardware:** Buzzer toggle, LED brightness.

### 6.3 Data Persistence & Statistics (NVS/Storage)
FocusLock tracks usage to provide historical insights on the Web UI. Accurate timestamps are provided by the hardware DS3231 RTC.

**Recorded Metrics:**
- **Completed Pomodoros:** Incremented only when transitioning from `WORK`/`WARNING` to `REST`.
- **Time Tracking:** Total focus duration, total rest duration.
- **Historical Data:** Data is structured in NVS to allow querying by Day, Week, and Month for visual display on the Web UI.