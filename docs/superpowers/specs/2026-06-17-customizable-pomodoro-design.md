# 2026-06-17 高度自定义番茄钟调度系统设计 (Customizable Pomodoro Scheduler)

## 1. 目标 (Goals)
实现一个基于时间的自动化番茄钟系统，支持用户根据周计划自定义工作时段、工作模式，并在特定时段自动开启/关闭番茄钟。

## 2. 核心逻辑 (Core Logic)
- **多模式层级 (Hierarchical Modes):** 支持预设多种番茄钟节奏（如“深度专注”、“常规”、“学习”）。
- **自动化执行 (Auto-Execution):** 到达预定时段自动开启番茄钟，无需手动确认。
- **善始善终原则 (Complete the Cycle):** 预定时段结束时，允许当前番茄钟循环（工作+休息）执行完毕后再退出。
- **自动超时保护 (Auto-Timeout):** 手动暂停超过 5 分钟后，自动终止当前专注，回归待机状态。
- **休眠显示 (Standby Mode):** 非工作时段自动进入待机状态，屏幕显示日期、时间和相关信息。

## 3. 架构设计 (Architecture)

### 3.1 存储策略 (Storage Strategy)
使用 SPIFFS 存储 `schedule.json` 配置文件。

```json
{
  "modes": [
    {
      "id": 0,
      "name": "Focus",
      "work_min": 50,
      "rest_min": 10,
      "warn_sec": 30
    }
  ],
  "calendar": {
    "mon": [
      { "start": "09:00", "end": "12:30", "mode_id": 0 },
      { "start": "14:00", "end": "17:30", "mode_id": 0 }
    ],
    "tue": [], 
    "wed": [], "thu": [], "fri": [], "sat": [], "sun": []
  }
}
```

### 3.2 系统组件 (System Components)
1.  **Scheduler Service (新组件):** 
    - 每分钟检查一次 RTC 时间。
    - 匹配当前的周日期和时间，查找是否处于定义的 `Schedule Block` 内。
    - 负责向 `Pomodoro Engine` 发送开启/停止指令。
2.  **Pomodoro Engine (现有组件增强):**
    - 增加 `AUTO_START` 事件处理。
    - 增加 `PENDING_EXIT` 标记，用于处理“善始善终”逻辑。
3.  **Config Manager (增强):**
    - 增加 SPIFFS 初始化和 JSON 解析（使用 cJSON 库）。
4.  **OLED Service (增强):**
    - 增加待机页面（Standby Page），显示大字体时间、日期和下一段计划预览。

## 4. 关键流程 (Key Flows)

### 4.1 自动开启流程
1. `Scheduler` 每分钟获取 `rtc_get_time()`。
2. 匹配 `calendar[today]` 下的所有块。
3. 如果 `now == block.start`，调用 `pomodoro_engine_start(block.mode_id)`。

### 4.2 边界退出流程
1. 如果 `now == block.end`：
   - 引擎状态为 `STATE_WORK` 或 `STATE_REST`：设置 `pending_exit = true`。
   - 引擎完成当前 `REST` 阶段后，检查 `pending_exit`，若为真则调用 `stop()` 并进入待机。

### 4.3 超时逻辑
1. 引擎进入 `STATE_PAUSE` 时启动定时器。
2. 若 5 分钟内未收到 `EVT_RESUME`，自动发送 `EVT_STOP` 并记录本次专注失败。

## 5. 错误处理 (Error Handling)
- **JSON 解析失败:** 默认回退到手动模式，并提示配置错误。
- **RTC 丢失时间:** 若 RTC 未同步，禁用自动调度功能，显示“Time Not Set”。
- **时段重叠:** 若配置中存在重叠时段，以第一个匹配到的时段为准。

## 6. 测试策略 (Testing)
- **单元测试:** 编写测试脚本模拟不同时间点，验证 `Scheduler` 的决策逻辑。
- **压力测试:** 频繁修改配置文件，观察 SPIFFS 的读写稳定性和内存占用。
- **边界测试:** 模拟跨天（23:59 -> 00:00）和时段结束瞬间的引擎状态切换。
