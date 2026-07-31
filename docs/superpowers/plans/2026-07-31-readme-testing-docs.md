# Developer README and Testing Guide Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create an accurate root developer guide and a separate, executable firmware testing procedure for the board2 project.

**Architecture:** `README.md` is the developer entry point and explains control flow, RTOS responsibilities, module boundaries, and build usage. `TESTING.md` is the operational verification guide and separates automated checks from tests that require a powered board and instruments.

**Tech Stack:** Markdown, Mermaid, STM32CubeMX/HAL, CMSIS-RTOS2/FreeRTOS, CMake, Ninja, GNU Arm Embedded Toolchain, PowerShell host tests.

---

## File Structure

- Create `README.md`: developer-oriented project entry point and architecture guide.
- Create `TESTING.md`: automated and hardware test procedure with expected results.
- Read `docs/board2-at-uart-pwm.md`: existing protocol reference used to verify commands and responses.
- Read `App/Inc/*.h`, `App/Src/*.c`, `Core/Src/freertos.c`, `Core/Src/stm32f1xx_it.c`, `Core/Src/tim.c`, and `Core/Src/usart.c`: implementation sources used as the documentation authority.

### Task 1: Write the developer README

**Files:**
- Create: `README.md`
- Reference: `docs/superpowers/specs/2026-07-31-readme-testing-docs-design.md`
- Reference: `docs/board2-at-uart-pwm.md`

- [ ] **Step 1: Reconfirm implementation constants and public behavior**

Run:

```powershell
rg -n "APP_|stack_size|priority|ReceiveToIdle|PSC|Period|UART[123]_IRQn|configTOTAL_HEAP_SIZE" App Core CMakeLists.txt stem-hub-board2.ioc
```

Expected: the output confirms 128-byte line storage, 32-byte tunnel payloads, 64-byte RX chunks, 256-byte rings, 6 KiB FreeRTOS heap, three RTOS tasks, UART receive-to-idle interrupts, and TIM4 PWM settings.

- [ ] **Step 2: Create the README using the approved section structure**

Write `README.md` with these concrete sections:

```markdown
# stem-hub-board2
## 快速开始
## 功能范围
## 硬件资源与默认状态
## 软件控制逻辑
## 任务与中断模型
## 代码架构
## AT 与透传接口
## 关键配置
## 构建产物
## 开发注意事项
## 测试与进一步阅读
```

The software-control section must include a Mermaid flowchart connecting USART IRQ callbacks, 256-byte rings, `atTask`, `bridgeTask`, output control, and UART transmit serialization. Describe that ISR code only copies data, marks overflow, signals a semaphore, and rearms reception.

- [ ] **Step 3: Check README links and implementation names**

Run:

```powershell
rg -n "App_|UART|NMOS|PWM|README|TESTING|board2-at-uart-pwm" README.md
```

Expected: every module and linked document uses a real repository name; the README links to `TESTING.md` and `docs/board2-at-uart-pwm.md`.

### Task 2: Write the executable testing procedure

**Files:**
- Create: `TESTING.md`
- Reference: `tests/run_host_tests.ps1`
- Reference: `CMakePresets.json`
- Reference: `docs/board2-at-uart-pwm.md`

- [ ] **Step 1: Create ordered automatic and hardware test sections**

Write `TESTING.md` with these sections:

```markdown
# 板2固件测试流程
## 测试范围与当前状态
## 测试设备与接线
## 自动化测试
## 固件构建验证
## 上电前检查
## 上电默认状态
## AT 协议测试
## NMOS 输出测试
## PWM 输出测试
## UART 透传测试
## 异常与恢复测试
## 验收记录
```

Each hardware test contains prerequisites, exact input, expected UART response or pin waveform, and a checkbox. Explicitly state that hardware tests are pending until performed on a connected board.

- [ ] **Step 2: Include reproducible commands and success criteria**

Document these repository-root commands:

```powershell
.\tests\run_host_tests.ps1
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

Expected host result: five `PASS` lines. Expected firmware result: `stem-hub-board2.elf` links successfully and the memory report remains within 20 KiB RAM and 64 KiB Flash.

- [ ] **Step 3: Cover every public command and error**

Verify that `TESTING.md` includes all of:

```text
AT+NMOS1/2/3=ON|OFF
AT+PWM=0..100
AT+UART2=ON|OFF
AT+UART3=ON|OFF
AT+UART2&3=ON|OFF
AT+UARTTX=<HEX>
OK
+ERROR:PARSE
+ERROR:RANGE
+ERROR:UART_DISABLED
+ERROR:UART_TX
+ERROR:LINE_TOO_LONG
+ERROR:RX_OVERFLOW
```

Run:

```powershell
rg -n "NMOS|PWM|UART2|UART3|UARTTX|ERROR:|RX_OVERFLOW" TESTING.md
```

Expected: each command family and each error response appears in at least one executable test case or expected-results table.

### Task 3: Validate and commit the documentation

**Files:**
- Validate: `README.md`
- Validate: `TESTING.md`

- [ ] **Step 1: Scan for placeholders and broken local links**

Run:

```powershell
rg -n "TB[D]|TO[D]O|待补充|占位" README.md TESTING.md
Test-Path README.md
Test-Path TESTING.md
Test-Path docs\board2-at-uart-pwm.md
```

Expected: no placeholder matches and all three `Test-Path` calls return `True`.

- [ ] **Step 2: Validate Markdown whitespace and preserve unrelated files**

Run:

```powershell
git diff --check -- README.md TESTING.md
git status --short
```

Expected: no whitespace errors; `.clangd`, `.settings/`, and `.vscode/` remain untracked and unchanged.

- [ ] **Step 3: Re-run automated verification quoted by the documentation**

Run:

```powershell
.\tests\run_host_tests.ps1
```

Expected:

```text
PASS test_at_protocol
PASS test_line_reader
PASS test_ring_buffer
PASS test_output_math
PASS test_uart_tunnel
```

- [ ] **Step 4: Commit only the two user-facing documents**

Run:

```powershell
git add README.md TESTING.md
git commit -m "docs: add developer guide and test procedure"
```

Expected: one commit containing only `README.md` and `TESTING.md`; unrelated IDE files remain outside the commit.
