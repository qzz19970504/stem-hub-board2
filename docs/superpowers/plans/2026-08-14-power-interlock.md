# Buck Power Interlock Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add PB3/PB12 Buck control, AT power commands, status query, and safe NMOS/PWM interlocks, then verify on host and hardware.

**Architecture:** Keep parsing pure in `app_at_protocol`, centralize output state and interlocks in `app_output`, and format status through a small pure `app_status` module. Synchronize CubeMX metadata and generated GPIO code, then verify by host tests, firmware builds, ST-Link flash, and UART1/485 interaction.

**Tech Stack:** STM32F103 HAL, CMSIS-RTOS2/FreeRTOS, C11, CMake/Ninja, GNU Arm Embedded, STM32CubeProgrammer, PowerShell.

---

### Task 1: Protocol and state tests

**Files:** `tests/test_at_protocol.c`, `tests/test_output_state.c`, `tests/test_status.c`, `tests/run_host_tests.ps1`

- [ ] Add failing tests for `AT+12V`, `AT+18V`, `AT+STATUS=?`, default state, safe-off cascades, denied ON/nonzero PWM, and exact status text.
- [ ] Run `tests/run_host_tests.ps1` and confirm failures are caused by missing APIs/commands.

### Task 2: Pure protocol and state implementation

**Files:** `App/Inc/app_at_protocol.h`, `App/Src/app_at_protocol.c`, `App/Inc/app_output_state.h`, `App/Src/app_output_state.c`, `App/Inc/app_status.h`, `App/Src/app_status.c`, `CMakeLists.txt`

- [ ] Add power/status command variants and parsing.
- [ ] Implement testable state transitions and exact status encoding.
- [ ] Run all host tests until green with `-Wall -Wextra -Werror`.
- [ ] Commit the pure behavior with a Chinese commit message.

### Task 3: HAL integration and CubeMX sync

**Files:** `App/Inc/app_output.h`, `App/Src/app_output.c`, `App/Src/app_tasks.c`, `Core/Inc/main.h`, `Core/Src/gpio.c`, `stem-hub-board2.ioc`

- [ ] Configure PB3/PB12 as high-initial push-pull outputs and keep SWD enabled/JTAG disabled.
- [ ] Integrate the state transitions with GPIO/TIM and map denied operations to `+ERROR:12V_DISABLED` or `+ERROR:18V_DISABLED`.
- [ ] Ensure startup state is both Bucks OFF, all NMOS OFF, PWM 0%.
- [ ] Commit integration with a Chinese commit message.

### Task 4: Documentation

**Files:** `README.md`, `TESTING.md`, `docs/board2-at-uart-pwm.md`, `板2_需求文档.md`

- [ ] Add only the new pins, commands, status format, interlocks, and short hardware checks.
- [ ] Run `git diff --check` and scan for stale command/error lists.
- [ ] Commit documentation with a Chinese commit message.

### Task 5: Automated and hardware verification

**Files:** build outputs only

- [ ] Run all host tests.
- [ ] Clean-build Debug and Release with Cube bundled tools and record memory use.
- [ ] Flash the Debug ELF using STM32CubeProgrammer under reset and require verification success.
- [ ] Identify the connected UART/485 COM port, execute the AT acceptance sequence, and record exact responses.
- [ ] Confirm PB3/PB12 default and toggled levels through available measurement/debug evidence.

### Task 6: Review and merge

- [ ] Review the complete diff for safety, parser edge cases, CubeMX consistency, and test coverage.
- [ ] Merge with `--no-ff` into `master` using a Chinese merge message.
- [ ] Re-run host tests and builds on the merge commit.
- [ ] Remove the owned worktree and merged branch after successful verification.
