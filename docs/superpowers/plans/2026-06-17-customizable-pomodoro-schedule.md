# 高度自定义番茄钟调度系统实施计划 (含 Web UI)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现一个基于 JSON 配置文件的自动化番茄钟调度系统，并同步更新 Web UI 以支持图形化配置多时段周计划。

**Architecture:** 采用“调度器-引擎-UI”解耦架构。`Scheduler Service` 驱动 `Pomodoro Engine`；`Config Manager` 负责 SPIFFS JSON 读写；Web Server 提供新的 RESTful 接口供 Web UI 访问 `schedule.json`。

**Tech Stack:** ESP-IDF (C), SPIFFS, cJSON, Vanilla JS (Web UI), ESP32-S3.

---

### Task 1: 基础设施 - SPIFFS 与 cJSON 集成
**Files:**
- Modify: `main/CMakeLists.txt`
- Modify: `main/config_mgr.c`
- Create: `main/spiffs_manager.c`

- [ ] **Step 1: 初始化 SPIFFS 挂载**
- [ ] **Step 2: 验证 `data/` 目录能正确打包进闪存**

### Task 2: 后端 - JSON 解析与配置管理
**Files:**
- Modify: `main/config_mgr.h`
- Modify: `main/config_mgr.c`

- [ ] **Step 1: 实现从 SPIFFS 读取/写入 `schedule.json` 的 API**
- [ ] **Step 2: 实现 C 结构体与 cJSON 之间的双向转换逻辑**

### Task 3: 后端 - REST API 扩展
**Files:**
- Modify: `main/web_server.c`

- [ ] **Step 1: 新增 `GET /api/schedule` 接口**，返回当前的 `schedule.json` 内容。
- [ ] **Step 2: 新增 `POST /api/schedule` 接口**，接收并保存新的 JSON 配置，并通知 `Scheduler Service` 重新加载。

### Task 4: 前端 - Web UI 调度编辑器实现
**Files:**
- Modify: `main/web_ui.html`

- [ ] **Step 1: 增加“计划编辑器”面板**。使用表格或列表展示周一至周日的时段。
- [ ] **Step 2: 实现动态添加/删除时段的 JS 逻辑**。
- [ ] **Step 3: 实现配置保存逻辑**，通过 `fetch(POST)` 将数据发送到后端。

### Task 5: 核心 - Scheduler Service 实现
**Files:**
- Create: `main/scheduler_service.c`

- [ ] **Step 1: 实现每分钟检查逻辑**。
- [ ] **Step 2: 实现“计划内自动开启”与“计划结束自动标记退出”的信号机制**。

### Task 6: 增强 - Pomodoro Engine 行为调整
**Files:**
- Modify: `main/pomodoro_engine.c`

- [ ] **Step 1: 实现 `PENDING_EXIT` 逻辑**（允许走完最后一轮工作+休息）。
- [ ] **Step 2: 实现 5 分钟暂停自动超时**。

### Task 7: 增强 - OLED 待机与总装
**Files:**
- Modify: `main/oled_service.c`
- Modify: `main/main.c`

- [ ] **Step 1: 实现大字体时间待机页面**。
- [ ] **Step 2: 全系统联调测试**。
