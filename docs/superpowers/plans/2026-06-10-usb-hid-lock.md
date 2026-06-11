# USB HID Lock Screen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the USB HID Keyboard service to automatically lock the user's screen when entering the REST state.

**Architecture:** Use the `espressif/esp_tinyusb` component to simulate a HID Keyboard. A dedicated task will monitor the status queue and dispatch key combinations based on NVS configuration.

**Tech Stack:** C, ESP-IDF, TinyUSB component.

---

### Task 1: TinyUSB Integration and Configuration

**Files:**
- Modify: `main/idf_component.yml`
- Modify: `main/CMakeLists.txt`

- [ ] **Step 1: Add TinyUSB dependency**

Update `main/idf_component.yml` to include `espressif/esp_tinyusb`.

- [ ] **Step 2: Update CMakeLists.txt**

Modify `main/CMakeLists.txt` to include `esp_tinyusb` in `PRIV_REQUIRES`.

- [ ] **Step 3: Verify Build (Component Download)**

Run `idf.py build` to trigger the download of the new component.

---

### Task 2: HID Keyboard Service Implementation

**Files:**
- Create: `main/usb_hid_service.h`
- Create: `main/usb_hid_service.c`
- Modify: `main/main.c`

- [ ] **Step 1: Create Header**

Create `main/usb_hid_service.h` defining `usb_hid_service_init(QueueHandle_t q)`.

- [ ] **Step 2: Implement USB Initialization and Task**

Create `main/usb_hid_service.c`. Initialize TinyUSB as a HID Keyboard. Implement a task that peeks the status queue. When state transitions to `STATE_REST`, trigger the lock sequence.

- [ ] **Step 3: Implement Key Sequence Logic**

Implement the logic to send keys. Since parsing "Win+L" is complex for a first pass, start by hardcoding the Windows lock key (GUI + L) and support custom shortcuts in a follow-up.

- [ ] **Step 4: Integrate into main.c**

Initialize the service in `app_main`.

---

### Task 3: Verification11

- [ ] **Step 1: Build and Flash**
- [ ] **Step 2: Physical Test**
