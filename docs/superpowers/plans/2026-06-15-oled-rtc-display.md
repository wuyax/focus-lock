# OLED RTC Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Update OLED display to use centralized I2C and show RTC time.

**Architecture:** Refactor `oled_service_init` to use `i2c_bus_handle` and update `oled_task` to fetch and display time from `rtc_service`.

**Tech Stack:** ESP-IDF, u8g2, C.

---

### Task 1: Update main.c with I2C and RTC Initialization

**Files:**
- Modify: `main/main.c`

- [ ] **Step 1: Add includes and init calls**
Ensure `i2c_manager_init()` and `rtc_service_init()` are called before services that use them.

### Task 2: Refactor oled_service.c for Centralized I2C

**Files:**
- Modify: `main/oled_service.c`

- [ ] **Step 1: Update includes**
Add `#include "i2c_manager.h"` and `#include "rtc_service.h"`.

- [ ] **Step 2: Update oled_service_init**
Replace local I2C initialization with `i2c_bus_handle`.

### Task 3: Update oled_task to Display RTC Time

**Files:**
- Modify: `main/oled_service.c`

- [ ] **Step 1: Modify oled_task loop**
Fetch time via `rtc_get_time()` and draw it on the OLED.

### Task 4: Verification and Commit

- [ ] **Step 1: Verify changes**
- [ ] **Step 2: Commit**
"feat: display real-time clock on OLED"
