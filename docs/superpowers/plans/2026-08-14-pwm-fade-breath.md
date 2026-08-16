# PWM Fade and Breath Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add non-blocking Gamma-corrected PWM fades, persistent fade timing, and an exclusive breath-test AT mode.

**Architecture:** Keep timing and Gamma behavior in a host-testable pure state machine. Keep Flash record validation pure and isolate HAL Flash access behind a storage adapter; wire both through the existing output and task modules.

**Tech Stack:** C11, STM32 HAL, CMSIS-RTOS2/FreeRTOS, CMake/Ninja, host GCC.

---

### Task 1: AT protocol contracts

**Files:** `App/Inc/app_at_protocol.h`, `App/Src/app_at_protocol.c`, `tests/test_at_protocol.c`

- [ ] Add failing cases for `AT+PWM_TIME=0..10000` and `AT+BREATH_TEST=ON|OFF`, including range and strict-format failures.
- [ ] Run `tests/run_host_tests.ps1` and confirm the new parser cases fail.
- [ ] Add command types and strict parsing, then rerun the suite to green.

### Task 2: Pure fade and Gamma state machine

**Files:** `App/Inc/app_pwm_fade.h`, `App/Src/app_pwm_fade.c`, `tests/test_pwm_fade.c`, `tests/run_host_tests.ps1`

- [ ] Write failing tests for instant, rising, falling and interrupted fades; Gamma endpoints/monotonicity; breath phase reversal and saved-target restore.
- [ ] Implement a 10 ms tick state machine using elapsed/total time interpolation and a Gamma 2.2 lookup table.
- [ ] Run all host tests and confirm they pass without HAL mocks.

### Task 3: Persistent setting records

**Files:** `App/Inc/app_settings_record.h`, `App/Src/app_settings_record.c`, `tests/test_settings_record.c`, `App/Inc/app_settings_storage.h`, `App/Src/app_settings_storage.c`, `STM32F103XX_FLASH.ld`

- [ ] Write failing tests for defaults, valid record selection, sequence rollover and corrupt record rejection.
- [ ] Implement the record encoder/validator and HAL Flash page rotation adapter.
- [ ] Reserve the final 1 KiB Flash page by reducing the application region to 63 KiB.

### Task 4: Runtime integration and safety

**Files:** `App/Inc/app_output.h`, `App/Inc/app_output_state.h`, `App/Src/app_output.c`, `App/Src/app_output_state.c`, `App/Src/app_tasks.c`, `App/Src/app_status.c`, `Core/Src/freertos.c`

- [ ] Route AT PWM commands to target changes and add PWM time/breath handlers with specific errors.
- [ ] Tick the fade from system task every 10 ms and serialize output state access.
- [ ] Make 18V OFF immediately cancel breath and force target/current/CCR to zero.
- [ ] Extend status output and its host tests.

### Task 5: Documentation, firmware and hardware verification

**Files:** `README.md`, `TESTING.md`, `docs/board2-at-uart-pwm.md`, `板2_需求文档.md`, `CMakeLists.txt`

- [ ] Document commands, defaults, persistence, Gamma semantics, breath exclusivity and emergency power-off behavior.
- [ ] Run host tests and clean Debug/Release builds with zero warnings and valid memory usage.
- [ ] Flash Debug through ST-Link, then use COM12 and SWD to verify fade progression, breathing, lockout, power-off and reboot persistence.
- [ ] Review the final diff and create one Chinese feature commit.
