# UART Transparent Mode Design

## Goal

Replace the six UART bridge enable/disable commands with three commands that enter an exclusive transparent mode. While transparent mode is active, UART1 payload is forwarded without AT parsing, and a guarded `+++` sequence returns the device to AT mode.

## Command interface

- `AT+TRANS=1\r\n` selects UART2 as the transparent target.
- `AT+TRANS=2\r\n` selects UART3 as the transparent target.
- `AT+TRANS=1&2\r\n` selects both UART2 and UART3.
- A valid `AT+TRANS` command returns `OK\r\n` before transparent data begins.
- The old `AT+UART2=ON/OFF`, `AT+UART3=ON/OFF`, and `AT+UART2&3=ON/OFF` commands are removed and return `+ERROR:PARSE\r\n` in AT mode.
- Leaving transparent mode clears the active targets. A new `AT+TRANS` command is required to re-enter it.

The command values `1` and `2` name the two downstream channels: channel 1 is MCU UART2, and channel 2 is MCU UART3. UART1 remains the host-side port.

## Modes and data flow

The UART1 processing path has two mutually exclusive states:

1. In AT mode, complete CRLF frames are passed to `AppAtProtocol_ParseFrame`. Non-AT frames are not forwarded because no transparent target is active.
2. A successful `AT+TRANS` command selects the target mask and switches to transparent mode.
3. In transparent mode, UART1 bytes bypass the line reader and the full AT parser. Text such as `AT+PWM=100\r\n` is ordinary payload and is forwarded unchanged.
4. UART2 and UART3 data continues to return through UART1 using the existing `+UART2RX:<HEX>\r\n` and `+UART3RX:<HEX>\r\n` event format while the corresponding target is active.
5. A valid guarded `+++` switches back to AT mode, clears both target bits, and produces no forwarded bytes. The device sends `OK\r\n` after the exit is confirmed so the host can identify the mode transition.

## Guarded escape detection

The escape detector recognizes `+++` only when both guard conditions are met:

- at least 1 ms of UART1 receive silence before the first `+`;
- at least 1 ms of UART1 receive silence after the third `+`.

The existing `HAL_UARTEx_ReceiveToIdle_IT` path is retained. At 9600 baud, one 8N1 character occupies about 1.04 ms, so a UART IDLE boundary is a conservative hardware-backed proof of the requested 1 ms silence. The runtime records UART1 receive chunks as idle-delimited units and notifies the AT task on both incoming data and the escape confirmation deadline.

In transparent mode:

- A chunk other than exactly three plus bytes is forwarded immediately.
- An idle-delimited chunk containing exactly `+++` becomes a pending escape candidate instead of being forwarded.
- If no UART1 byte arrives during the following 1 ms confirmation interval, the escape succeeds.
- If another byte arrives before confirmation, the pending `+++` is forwarded first, followed by the new bytes in their original order.
- `abc+++def`, `++++`, and any `+++` without both guard intervals remain ordinary payload and must not lose, reorder, or duplicate bytes.

This deliberately uses the UART idle boundary rather than rewriting reception as per-byte interrupts. It preserves the current interrupt/ring-buffer/FreeRTOS architecture and errs on the safe side: marginal timing does not cause an unintended escape.

## Component changes

- `app_at_protocol` parses only the three new `AT+TRANS` values and represents them as a target selection command.
- A small host-testable transparent-mode processor owns mode state, escape candidate buffering, guard-time decisions, and forwarding actions. It performs no HAL or RTOS calls.
- `app_tasks` routes UART1 data either to the existing AT line path or to the transparent-mode processor and performs the processor's requested sends and mode transitions.
- `app_runtime` exposes the idle-delimited UART1 chunks and a timed wait needed to confirm post-escape silence without busy polling.
- Existing bridge target masks and UART2/UART3 return-event handling are reused.

## Error and overflow behavior

- Malformed `AT+TRANS` values return `+ERROR:PARSE\r\n` in AT mode.
- UART1 ring-buffer overflow resets the active parser/escape candidate, exits transparent mode, clears the target mask, and reports `+ERROR:RX_OVERFLOW\r\n`.
- UART transmission failures use the existing runtime send status behavior; they do not silently change mode.
- A failed escape candidate is always replayed as payload before later bytes.

## Verification

Host tests cover:

- parsing all three new commands and rejecting all six old commands;
- entering each target selection and leaving transparent mode;
- AT-like payload bypassing the AT parser;
- valid `+++` with both 1 ms guards;
- rejection and exact replay when either guard is missing;
- `abc+++def`, `++++`, fragmented plus bytes, binary payload, and ordering across chunks;
- existing AT, output, ring-buffer, status, and UART event regressions.

Firmware verification covers Debug and Release builds with warnings treated as errors. Hardware verification programs and verifies the ELF through the detected ST-Link, then uses UART1 on COM12 at 9600 8N1 to check the three entry commands, raw forwarding, AT-command suppression during transparency, invalid/valid escape timing, and return to AT mode. The final merged `master` is rebuilt and its host tests are run again.
