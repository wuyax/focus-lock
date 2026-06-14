# Standardize Documentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Standardize Doxygen headers across all source and header files in the `main/` directory to professionalize the codebase.

**Architecture:** Add `@file` headers to `.c` files and `@brief` descriptions to public functions in `.h` files, matching the established style in `pomodoro_engine.c`.

**Tech Stack:** C, Doxygen.

---

### Task 1: Core and Utility Services

**Files:**
- Modify: `main/i2c_manager.c`, `main/i2c_manager.h`
- Modify: `main/config_mgr.c`, `main/config_mgr.h`
- Modify: `main/shortcut_parser.c`, `main/shortcut_parser.h`

- [ ] **Step 1: Document i2c_manager**
Add `@file` header to `main/i2c_manager.c` and `@brief` to `i2c_manager_init` in `main/i2c_manager.h`.

- [ ] **Step 2: Document config_mgr**
Add `@file` header to `main/config_mgr.c` and `@brief` to all public functions in `main/config_mgr.h`.

- [ ] **Step 3: Document shortcut_parser**
Add `@file` header to `main/shortcut_parser.c` and `@brief` to all public functions in `main/shortcut_parser.h`.

- [ ] **Step 4: Commit**
```bash
git add main/i2c_manager.* main/config_mgr.* main/shortcut_parser.*
git commit -m "docs: document core and utility services"
```

---

### Task 2: Input/Output Services

**Files:**
- Modify: `main/button_service.c`, `main/button_service.h`
- Modify: `main/buzzer_service.c`, `main/buzzer_service.h`
- Modify: `main/rgb_service.c`, `main/rgb_service.h`

- [ ] **Step 1: Document button_service**
Add `@file` header to `main/button_service.c` and `@brief` to `button_service_init` in `main/button_service.h`.

- [ ] **Step 2: Document buzzer_service**
Add `@file` header to `main/buzzer_service.c` and `@brief` to all public functions in `main/buzzer_service.h`.

- [ ] **Step 3: Document rgb_service**
Add `@file` header to `main/rgb_service.c` and `@brief` to all public functions in `main/rgb_service.h`.

- [ ] **Step 4: Commit**
```bash
git add main/button_service.* main/buzzer_service.* main/rgb_service.*
git commit -m "docs: document I/O services"
```

---

### Task 3: Display and Time Services

**Files:**
- Modify: `main/oled_service.c`, `main/oled_service.h`
- Modify: `main/rtc_service.c`, `main/rtc_service.h`

- [ ] **Step 1: Document oled_service**
Add `@file` header to `main/oled_service.c` and `@brief` to all public functions in `main/oled_service.h`.

- [ ] **Step 2: Document rtc_service**
Add `@file` header to `main/rtc_service.c` and `@brief` to all public functions in `main/rtc_service.h`.

- [ ] **Step 3: Commit**
```bash
git add main/oled_service.* main/rtc_service.*
git commit -m "docs: document display and time services"
```

---

### Task 4: Network and Web Services

**Files:**
- Modify: `main/network_service.c`, `main/network_service.h`
- Modify: `main/dns_server.c`, `main/dns_server.h`
- Modify: `main/web_server.c`, `main/web_server.h`

- [ ] **Step 1: Document network_service**
Add `@file` header to `main/network_service.c` and `@brief` to `network_service_init` in `main/network_service.h`.

- [ ] **Step 2: Document dns_server**
Add `@file` header to `main/dns_server.c` and `@brief` to all public functions in `main/dns_server.h`.

- [ ] **Step 3: Document web_server**
Add `@file` header to `main/web_server.c` and `@brief` to all public functions in `main/web_server.h`.

- [ ] **Step 4: Commit**
```bash
git add main/network_service.* main/dns_server.* main/web_server.*
git commit -m "docs: document network and web services"
```

---

### Task 5: Core Logic and USB HID

**Files:**
- Modify: `main/pomodoro_engine.c`, `main/pomodoro_engine.h`
- Modify: `main/usb_hid_service.c`, `main/usb_hid_service.h`

- [ ] **Step 1: Document pomodoro_engine (Complete)**
Check `main/pomodoro_engine.c` and `main/pomodoro_engine.h` for any missing Doxygen comments and complete them.

- [ ] **Step 2: Document usb_hid_service**
Add `@file` header to `main/usb_hid_service.c` and `@brief` to all public functions in `main/usb_hid_service.h`.

- [ ] **Step 3: Commit**
```bash
git add main/pomodoro_engine.* main/usb_hid_service.*
git commit -m "docs: document core logic and USB HID services"
```

---

### Task 6: Final Verification

- [ ] **Step 1: Build check**
Run: `idf.py build` (if available)
Expected: Build succeeds.

- [ ] **Step 2: Final Commit**
```bash
git add main/*.c main/*.h
git commit -m "docs: final pass on Doxygen standardization"
```
