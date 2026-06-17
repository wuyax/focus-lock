# Task 7: OLED Standby and Final Assembly Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a beautiful standby page for the OLED display when the system is not in a Pomodoro session and perform final system integration.

**Architecture:** 
- Modify `oled_service.c` to detect `STATE_ADMIN` and render a standby page with large HH:MM time, weekday, and next scheduled block.
- Modify `main.c` to remove test code, load the actual schedule from SPIFFS, and ensure correct service initialization sequence.

**Tech Stack:** ESP-IDF, u8g2, SPIFFS, RTC.

---

### Task 1: Implement Standby Page in OLED Service

**Files:**
- Modify: `main/oled_service.c`

- [ ] **Step 1: Define helper to find next scheduled block**
  Add a helper function to find the next scheduled block for the current day or upcoming days.

- [ ] **Step 2: Update `oled_task` to handle `STATE_ADMIN`**
  Modify `oled_task` to switch between "Active session" view and "Standby" view based on `g_status.state`.

- [ ] **Step 3: Implement Standby View**
  - Use `u8g2_font_logisoso32_tf` for the big HH:MM time.
  - Display the weekday.
  - Display "Next: HH:MM" if a block is found.

- [ ] **Step 4: Verify OLED Standby Rendering**
  (This is a manual verification if hardware was available, but we can verify compilation and logic).
  Run: `idf.py build`
  Expected: Success.

### Task 2: Final System Integration in Main

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: Remove `test_schedule_config`**
  Remove the sample schedule generation code and its call.

- [ ] **Step 2: Update `app_main` to load actual schedule**
  Ensure `config_mgr_load_schedule` is called (or let `scheduler_service` handle it if it does).
  Actually, `scheduler_service_init` probably loads it, but we should make sure it's available.

- [ ] **Step 3: Ensure correct initialization order**
  1. Event loop
  2. SPIFFS (via `config_mgr_init`)
  3. I2C Manager
  4. RTC Service
  5. Pomodoro Engine
  6. Other services (OLED, RGB, Button, Buzzer, Scheduler)

- [ ] **Step 4: Verify Final Build**
  Run: `idf.py build`
  Expected: Success.
