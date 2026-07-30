#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "app_at_protocol.h"
#include "stm32f1xx_hal.h"

#define APP_BRIDGE_MASK_UART2 (1UL << 0U)
#define APP_BRIDGE_MASK_UART3 (1UL << 1U)

/** Create all RTOS synchronization objects used by the application. */
void App_RuntimeCreateObjects(void);

/** Start interrupt-to-idle reception on UART1. */
void App_RuntimeStartUart1Receive(void);

/** Start interrupt-to-idle reception on UART2 and UART3. */
void App_RuntimeStartBridgeReceive(void);

/** Wait until UART1 has received data. */
void App_RuntimeWaitForUart1Data(void);

/** Wait until UART2 or UART3 has received data. */
void App_RuntimeWaitForBridgeData(void);

/** Pop one received byte from UART1, UART2, or UART3. */
bool App_RuntimePopRxByte(uint8_t uart_index, uint8_t *byte);

/**
 * Consume an overflow marker and discard the affected port's queued bytes.
 */
bool App_RuntimeConsumeRxOverflow(uint8_t uart_index);

/** Discard queued received bytes for a UART. */
void App_RuntimeFlushRx(uint8_t uart_index);

/** Enable or disable one bridge target. Disabling also flushes its RX queue. */
void App_RuntimeSetBridgeEnabled(AppBridgeTarget target, bool enabled);

/** Return the current APP_BRIDGE_MASK_* bit set. */
uint32_t App_RuntimeGetBridgeMask(void);

/**
 * Serialize one bridge RX consume/send transaction against bridge disable.
 *
 * The bridge task must hold this lock while checking the enable mask, popping
 * UART2/3 bytes, and publishing the corresponding UART1 event.
 */
void App_RuntimeLockBridge(void);
void App_RuntimeUnlockBridge(void);

/** Send bytes under the shared UART transmit mutex. */
HAL_StatusTypeDef App_RuntimeSendBytes(UART_HandleTypeDef *uart,
                                       const uint8_t *bytes,
                                       uint16_t length);

/** Send a NUL-terminated text frame under the transmit mutex. */
HAL_StatusTypeDef App_RuntimeSendText(UART_HandleTypeDef *uart, const char *text);

#endif
