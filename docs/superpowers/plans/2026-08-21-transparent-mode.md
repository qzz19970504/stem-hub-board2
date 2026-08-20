# UART Transparent Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the old UART bridge switches with three exclusive transparent-mode entry commands and a lossless `+++` exit guarded by at least 1 ms of silence on each side.

**Architecture:** Keep the existing UART Receive-to-Idle, byte ring buffers, FreeRTOS tasks, and UART2/UART3 return events. Add a pure C transparent-mode state machine and a small SPSC queue that preserves UART1 idle-delimited chunk metadata, then route each UART1 chunk exclusively through either the AT line reader or the transparent processor.

**Tech Stack:** STM32F103 HAL, FreeRTOS/CMSIS-RTOS2, C11, GCC host tests, CMake/Ninja, STM32CubeProgrammer, ST-Link, FTDI COM12 at 9600 8N1.

---

## File map

- Create `App/Inc/app_transparent_mode.h` and `App/Src/app_transparent_mode.c`: HAL-free mode and guarded-escape state machine.
- Create `App/Inc/app_uart_chunk_queue.h` and `App/Src/app_uart_chunk_queue.c`: fixed-size single-producer/single-consumer metadata queue for UART1 Receive-to-Idle chunks.
- Create `tests/test_transparent_mode.c` and `tests/test_uart_chunk_queue.c`: behavior and boundary regression tests.
- Modify `App/Inc/app_at_protocol.h` and `App/Src/app_at_protocol.c`: parse only `AT+TRANS=1`, `2`, and `1&2` as transparent-mode entry commands.
- Modify `App/Inc/app_runtime.h` and `App/Src/app_runtime.c`: expose UART1 chunks with pre/post idle evidence and exact target selection/clear operations.
- Modify `App/Src/app_tasks.c`: mutually exclusive AT/transparent routing and exit acknowledgement.
- Modify `App/Inc/app_config.h`, `CMakeLists.txt`, and `tests/run_host_tests.ps1`: sizes and build/test registration.
- Modify `README.md`, `TESTING.md`, `docs/board2-at-uart-pwm.md`, and `板2_需求文档.md`: published protocol and hardware acceptance procedure.

### Task 1: Replace the bridge command parser

**Files:**
- Modify: `tests/test_at_protocol.c`
- Modify: `App/Inc/app_at_protocol.h`
- Modify: `App/Src/app_at_protocol.c`

- [ ] **Step 1: Write parser tests for the new interface**

Replace the old bridge assertions with:

```c
static void expect_transparent(const char *line, AppBridgeTarget expected_target)
{
    AppAtCommand command = parse_ok(line);
    assert(command.type == APP_AT_COMMAND_START_TRANSPARENT);
    assert(command.data.transparent.target == expected_target);
}

expect_transparent("AT+TRANS=1\r\n", APP_BRIDGE_TARGET_UART2);
expect_transparent("AT+TRANS=2\r\n", APP_BRIDGE_TARGET_UART3);
expect_transparent("AT+TRANS=1&2\r\n", APP_BRIDGE_TARGET_UART23);
assert(AppAtProtocol_ParseLine("AT+TRANS=\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+TRANS=3\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART2=ON\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART2=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART3=ON\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART3=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART2&3=ON\r\n", &command) == APP_AT_PARSE_INVALID);
assert(AppAtProtocol_ParseLine("AT+UART2&3=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
```

- [ ] **Step 2: Run the parser test and verify the expected failure**

Run: `& gcc -std=c11 -Wall -Wextra -Werror -I App/Inc tests/test_at_protocol.c App/Src/app_at_protocol.c -o build/host-tests/test_at_protocol.exe; & build/host-tests/test_at_protocol.exe`

Expected: compilation fails because `APP_AT_COMMAND_START_TRANSPARENT` and `data.transparent` do not exist.

- [ ] **Step 3: Implement the minimal parser change**

Change the public command model to:

```c
typedef enum
{
    APP_AT_COMMAND_NONE = 0,
    APP_AT_COMMAND_SET_NMOS,
    APP_AT_COMMAND_SET_PWM,
    APP_AT_COMMAND_SET_PWM_TIME,
    APP_AT_COMMAND_SET_BREATH_TEST,
    APP_AT_COMMAND_SET_POWER,
    APP_AT_COMMAND_GET_STATUS,
    APP_AT_COMMAND_START_TRANSPARENT,
    APP_AT_COMMAND_SEND_UART
} AppAtCommandType;

typedef struct
{
    AppBridgeTarget target;
} AppAtTransparentCommand;
```

Replace `AppAtProtocol_MatchBridge` with an exact `AT+TRANS=` value matcher that maps `1`, `2`, and `1&2` to the existing UART2, UART3, and combined internal targets. Do not retain the old ON/OFF prefixes.

- [ ] **Step 4: Run the parser test and the full host suite**

Run: `& .\tests\run_host_tests.ps1`

Expected: all registered tests print `PASS`.

- [ ] **Step 5: Commit the parser change**

```powershell
git add App/Inc/app_at_protocol.h App/Src/app_at_protocol.c tests/test_at_protocol.c
git commit -m "功能：替换透传入口命令"
```

### Task 2: Add the lossless transparent-mode state machine

**Files:**
- Create: `tests/test_transparent_mode.c`
- Create: `App/Inc/app_transparent_mode.h`
- Create: `App/Src/app_transparent_mode.c`
- Modify: `App/Inc/app_config.h`
- Modify: `tests/run_host_tests.ps1`

- [ ] **Step 1: Write focused state-machine tests**

Define tests around this API:

```c
AppTransparentMode mode;
AppTransparentResult result;
static const uint8_t escape[] = {'+', '+', '+'};

AppTransparentMode_Init(&mode);
AppTransparentMode_Enter(&mode, APP_BRIDGE_TARGET_UART2);
assert(AppTransparentMode_IsActive(&mode));
assert(AppTransparentMode_GetTarget(&mode) == APP_BRIDGE_TARGET_UART2);

assert(AppTransparentMode_ProcessChunk(
    &mode, escape, sizeof(escape), true, true, &result));
assert(result.exited);
assert(result.forward_length == 0U);
assert(!AppTransparentMode_IsActive(&mode));
```

Add independent assertions for AT-looking payload forwarding unchanged, missing pre-guard, missing post-guard followed by another byte, `abc+++def`, `++++`, fragmented `+`/`+`/`+`, a failed `++X` candidate, embedded NUL bytes, and re-entry for all three targets. Every failed candidate must compare its full output bytes and order with `memcmp`.

- [ ] **Step 2: Register and run the new test to verify RED**

Add this case to `tests/run_host_tests.ps1`:

```powershell
@{ Name = 'test_transparent_mode'; Sources = @('app_transparent_mode.c') }
```

Run: `& .\tests\run_host_tests.ps1`

Expected: `test_transparent_mode` compilation fails because the header and implementation are absent.

- [ ] **Step 3: Implement the public state-machine API**

Use these public types and functions:

```c
typedef struct
{
    bool active;
    AppBridgeTarget target;
    uint8_t escape_candidate[3];
    size_t escape_length;
    bool escape_has_pre_guard;
} AppTransparentMode;

typedef struct
{
    uint8_t forward[APP_UART_RX_CHUNK_SIZE + 3U];
    size_t forward_length;
    bool exited;
} AppTransparentResult;

void AppTransparentMode_Init(AppTransparentMode *mode);
void AppTransparentMode_Enter(AppTransparentMode *mode, AppBridgeTarget target);
void AppTransparentMode_Abort(AppTransparentMode *mode);
bool AppTransparentMode_IsActive(const AppTransparentMode *mode);
AppBridgeTarget AppTransparentMode_GetTarget(const AppTransparentMode *mode);
bool AppTransparentMode_ProcessChunk(AppTransparentMode *mode,
                                     const uint8_t *bytes,
                                     size_t length,
                                     bool silence_before,
                                     bool silence_after,
                                     AppTransparentResult *result);
```

The implementation starts an escape candidate only on a leading `+` with `silence_before == true`. It buffers at most three plus bytes, exits only when the third plus is the last byte and `silence_after == true`, and copies any failed candidate before all later input into `result.forward`. `Abort` clears the target and candidate.

- [ ] **Step 4: Run the focused test and full host suite**

Run: `& .\tests\run_host_tests.ps1`

Expected: `PASS test_transparent_mode` and all prior tests remain green.

- [ ] **Step 5: Commit the state machine**

```powershell
git add App/Inc/app_config.h App/Inc/app_transparent_mode.h App/Src/app_transparent_mode.c tests/test_transparent_mode.c tests/run_host_tests.ps1
git commit -m "功能：新增无损透传退出状态机"
```

### Task 3: Preserve UART1 idle chunk boundaries

**Files:**
- Create: `tests/test_uart_chunk_queue.c`
- Create: `App/Inc/app_uart_chunk_queue.h`
- Create: `App/Src/app_uart_chunk_queue.c`
- Modify: `App/Inc/app_config.h`
- Modify: `tests/run_host_tests.ps1`

- [ ] **Step 1: Write the metadata queue test**

Test FIFO order, wraparound, full detection, reset, and preservation of both guard flags with:

```c
AppUartChunkQueue queue;
AppUartChunk chunk;
AppUartChunkQueue_Init(&queue);
assert(AppUartChunkQueue_Push(&queue, 3U, true, true));
assert(AppUartChunkQueue_Pop(&queue, &chunk));
assert(chunk.length == 3U);
assert(chunk.silence_before);
assert(chunk.silence_after);
```

Fill exactly `APP_UART_CHUNK_QUEUE_CAPACITY` entries, assert the next push fails and sets overflow, then reset and assert the queue is empty and no longer overflowed.

- [ ] **Step 2: Register and run the test to verify RED**

Register:

```powershell
@{ Name = 'test_uart_chunk_queue'; Sources = @('app_uart_chunk_queue.c') }
```

Run: `& .\tests\run_host_tests.ps1`

Expected: compilation fails because `app_uart_chunk_queue.h` is absent.

- [ ] **Step 3: Implement a fixed-size SPSC queue**

Use a statically allocated queue with volatile producer and consumer sequence counters:

```c
typedef struct
{
    uint16_t length;
    bool silence_before;
    bool silence_after;
} AppUartChunk;

typedef struct
{
    AppUartChunk entries[APP_UART_CHUNK_QUEUE_CAPACITY];
    volatile size_t head_sequence;
    volatile size_t tail_sequence;
    volatile bool has_overflowed;
} AppUartChunkQueue;
```

Set `APP_UART_CHUNK_QUEUE_CAPACITY` to `8U`. Provide `Init`, `Push`, `Pop`, `HasOverflowed`, and `Reset`, following the sequence-counter pattern already used by `app_ring_buffer.c`.

- [ ] **Step 4: Run the queue test and full suite**

Run: `& .\tests\run_host_tests.ps1`

Expected: all tests, including `test_uart_chunk_queue`, print `PASS`.

- [ ] **Step 5: Commit the queue**

```powershell
git add App/Inc/app_config.h App/Inc/app_uart_chunk_queue.h App/Src/app_uart_chunk_queue.c tests/test_uart_chunk_queue.c tests/run_host_tests.ps1
git commit -m "功能：保留串口空闲分段信息"
```

### Task 4: Integrate exclusive routing with the STM32 runtime

**Files:**
- Modify: `App/Inc/app_runtime.h`
- Modify: `App/Src/app_runtime.c`
- Modify: `App/Src/app_tasks.c`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Build before integration and record the expected linker-neutral baseline**

Run: `cmake --build --preset Debug`

Expected: the pre-integration firmware builds; this establishes that subsequent compile failures come from the integration edits.

- [ ] **Step 2: Add runtime chunk and target APIs**

Expose:

```c
bool App_RuntimePopUart1Chunk(uint8_t *bytes,
                              size_t capacity,
                              size_t *length,
                              bool *silence_before,
                              bool *silence_after);
void App_RuntimeSelectBridgeTarget(AppBridgeTarget target);
void App_RuntimeClearBridgeTarget(void);
```

For UART1, `HAL_UARTEx_RxEventCallback` must query `HAL_UARTEx_GetRxEventType`. Record `silence_after` only for `HAL_UART_RXEVENT_IDLE`, carry it into the next chunk as `silence_before`, and enqueue metadata only after all chunk bytes have been pushed. Queue overflow participates in `App_RuntimeConsumeRxOverflow(1U)` and both the byte ring and metadata queue reset together.

`App_RuntimeSelectBridgeTarget` atomically replaces the full target mask instead of accumulating bits. `App_RuntimeClearBridgeTarget` clears both bits and flushes UART2/UART3 RX queues under the existing bridge mutex.

- [ ] **Step 3: Route UART1 exclusively by mode**

In `App_AtTask`, initialize one `AppTransparentMode`. For every popped UART1 chunk:

```c
if (AppTransparentMode_IsActive(&transparent_mode))
{
    AppTransparentMode_ProcessChunk(&transparent_mode,
                                    chunk,
                                    chunk_length,
                                    silence_before,
                                    silence_after,
                                    &result);
    App_AtForwardBytes(result.forward, result.forward_length);
    if (result.exited)
    {
        App_RuntimeClearBridgeTarget();
        (void)App_RuntimeSendText(&huart1, "OK\r\n");
    }
}
else
{
    App_AtConsumeBytes(&line_reader, chunk, chunk_length);
}
```

Implement `App_AtForwardBytes` as a static helper that sends a non-empty byte span to each bit currently present in `App_RuntimeGetBridgeMask`. Implement `App_AtConsumeBytes` as a static helper that pushes one chunk through the existing line reader and calls `App_AtProcessFrame` for each completed CRLF frame. Handle `APP_AT_COMMAND_START_TRANSPARENT` by sending `OK\r\n`, selecting the exact bridge target, and entering the state machine. During transparent mode no byte reaches `AppLineReader_Push` or `AppAtProtocol_ParseFrame`. On UART1 overflow, abort transparent mode, clear targets, reset the line reader, and send the existing overflow error.

- [ ] **Step 4: Register sources and compile to expose integration errors**

Add `app_transparent_mode.c` and `app_uart_chunk_queue.c` to `APP_SOURCES`, then run:

Run: `cmake --build --preset Debug`

Expected before fixes: compiler errors identify any signature or HAL event-type mismatch. Resolve only those integration errors, keeping all application sources under `-Wall -Wextra -Werror`.

- [ ] **Step 5: Run all host tests and both firmware builds**

Run:

```powershell
& .\tests\run_host_tests.ps1
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

Expected: every host test prints `PASS`; both build commands exit zero with `stem-hub-board2.elf` produced.

- [ ] **Step 6: Commit the integration**

```powershell
git add App/Inc/app_runtime.h App/Src/app_runtime.c App/Src/app_tasks.c CMakeLists.txt
git commit -m "功能：分离AT与透明传输模式"
```

### Task 5: Update the protocol documentation

**Files:**
- Modify: `README.md`
- Modify: `TESTING.md`
- Modify: `docs/board2-at-uart-pwm.md`
- Modify: `板2_需求文档.md`

- [ ] **Step 1: Replace the public command and behavior descriptions**

Document the three mappings, removal of all six old commands, exclusive AT/transparent states, unchanged raw bytes, guarded exit, failed-candidate replay, `OK\r\n` exit acknowledgement, 9600-baud UART-IDLE timing basis, and the fact that other AT strings are payload while transparent.

- [ ] **Step 2: Rewrite UART acceptance cases**

Replace old ON/OFF procedures with explicit checks for `AT+TRANS=1`, `2`, `1&2`, AT-looking payload suppression, `abc+++def`, missing pre/post guard, valid guarded escape, old-command rejection, and re-entry after escape.

- [ ] **Step 3: Check stale protocol references**

Run: `rg -n "AT\+UART2=|AT\+UART3=|AT\+UART2&3=" README.md TESTING.md docs 板2_需求文档.md`

Expected: old commands occur only in migration/rejection test text, never as supported commands.

- [ ] **Step 4: Commit documentation**

```powershell
git add README.md TESTING.md docs/board2-at-uart-pwm.md 板2_需求文档.md
git commit -m "文档：更新透明传输协议与验收步骤"
```

### Task 6: Program and verify the hardware

**Files:**
- Firmware: `build/Debug/stem-hub-board2.elf`

- [ ] **Step 1: Re-run final pre-flash verification**

Run:

```powershell
& .\tests\run_host_tests.ps1
cmake --build --preset Debug
git diff --check
git status --short --branch
```

Expected: all tests pass, build exits zero, no whitespace errors, and only the user's pre-existing `.clangd`, `.settings/`, and `.vscode/` remain untracked.

- [ ] **Step 2: Program with the detected ST-Link**

Run:

```powershell
& "$env:USERPROFILE\.agents\skills\stm32-cubemx-flash\scripts\flash_stm32.ps1" `
  -ProjectRoot 'D:\Codes\STM32\stem-hub-board2' `
  -Configuration Debug `
  -FirmwarePath 'D:\Codes\STM32\stem-hub-board2\build\Debug\stem-hub-board2.elf' `
  -ProbeSerial '37FF71064E573436947D1143'
```

Expected: STM32CubeProgrammer reports download verification success and resets the target.

- [ ] **Step 3: Exercise UART1 through COM12**

Open COM12 at 9600 8N1 with no flow control. For each target command, send the complete CRLF frame in one write and record exact received bytes. Verify the old commands return parse errors; each new command returns `OK`; `AT+STATUS=?\r\n` produces no UART1 AT response during transparency; `abc+++def` does not exit; a guarded `+++` returns `OK`; and `AT+STATUS=?\r\n` works again after exit.

If UART2/UART3 observability requires a loopback or another serial adapter, stop and ask for the exact wiring before changing any connection. Do not infer or move hardware wiring.

- [ ] **Step 4: Commit any evidence-only test-document updates if produced**

Stage only tracked acceptance-document changes and use `git diff --cached` before committing. Do not add local editor settings or generated build files.

### Task 7: Merge to master and verify the merged result

**Files:**
- No new source files beyond the commits above.

- [ ] **Step 1: Verify the feature branch is clean and all requirements are covered**

Run:

```powershell
git status --short --branch
git log --oneline master..codex/transparent-mode
& .\tests\run_host_tests.ps1
cmake --build --preset Debug
cmake --build --preset Release
```

Expected: only known user-owned untracked settings remain; every test and build passes.

- [ ] **Step 2: Merge locally as explicitly requested**

Run:

```powershell
git switch master
git merge --no-ff codex/transparent-mode -m "合并：新增互斥透明传输模式"
```

Do not pull or push unless the user separately requests network synchronization.

- [ ] **Step 3: Verify master after the merge**

Run:

```powershell
& .\tests\run_host_tests.ps1
cmake --build --preset Debug
cmake --build --preset Release
git diff --check HEAD^ HEAD
git status --short --branch
```

Expected: tests and both builds pass on `master`, the merge contains no whitespace errors, and user-owned untracked settings are unchanged.

- [ ] **Step 4: Report the final evidence**

Report the feature and merge commit IDs, test count, Debug/Release build result, firmware path, ST-Link serial/device, program/verify/reset result, COM12 cases exercised, and any hardware path that could not be observed. Keep `codex/transparent-mode` unless the user explicitly requests branch deletion.
