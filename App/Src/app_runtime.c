#include "app_runtime.h"

#include <string.h>

#include "app_config.h"
#include "app_ring_buffer.h"
#include "app_uart_chunk_queue.h"
#include "cmsis_os.h"
#include "main.h"
#include "task.h"
#include "usart.h"

typedef struct
{
    UART_HandleTypeDef *uart;
    uint8_t uart_index;
    uint8_t receive_chunk[APP_UART_RX_CHUNK_SIZE];
    uint8_t ring_storage[APP_UART_RING_BUFFER_SIZE];
    AppRingBuffer ring;
    AppUartChunkQueue chunk_queue;
    bool next_chunk_silence_before;
} AppUartRuntime;

static AppUartRuntime app_uart_runtimes[] = {
    {.uart = &huart1, .uart_index = 1U},
    {.uart = &huart2, .uart_index = 2U},
    {.uart = &huart3, .uart_index = 3U},
};

static osSemaphoreId_t app_uart1_rx_semaphore;
static osSemaphoreId_t app_bridge_rx_semaphore;
static osMutexId_t app_uart_tx_mutex;
static osMutexId_t app_bridge_mutex;
static osEventFlagsId_t app_bridge_flags;

static AppUartRuntime *App_RuntimeFindUartByIndex(uint8_t uart_index)
{
    size_t runtime_index;

    for (runtime_index = 0U;
         runtime_index < (sizeof(app_uart_runtimes) / sizeof(app_uart_runtimes[0]));
         ++runtime_index)
    {
        if (app_uart_runtimes[runtime_index].uart_index == uart_index)
        {
            return &app_uart_runtimes[runtime_index];
        }
    }

    return NULL;
}

static AppUartRuntime *App_RuntimeFindUartByHandle(UART_HandleTypeDef *uart)
{
    size_t runtime_index;

    for (runtime_index = 0U;
         runtime_index < (sizeof(app_uart_runtimes) / sizeof(app_uart_runtimes[0]));
         ++runtime_index)
    {
        if (app_uart_runtimes[runtime_index].uart == uart)
        {
            return &app_uart_runtimes[runtime_index];
        }
    }

    return NULL;
}

static void App_RuntimeArmReceive(AppUartRuntime *runtime)
{
    if ((runtime == NULL)
        || (HAL_UARTEx_ReceiveToIdle_IT(runtime->uart,
                                       runtime->receive_chunk,
                                       sizeof(runtime->receive_chunk)) != HAL_OK))
    {
        Error_Handler();
    }
}

static void App_RuntimeFailFastIfObjectMissing(void *object)
{
    if (object == NULL)
    {
        Error_Handler();
    }
}

void App_RuntimeCreateObjects(void)
{
    size_t runtime_index;

    for (runtime_index = 0U;
         runtime_index < (sizeof(app_uart_runtimes) / sizeof(app_uart_runtimes[0]));
         ++runtime_index)
    {
        if (!AppRingBuffer_Init(&app_uart_runtimes[runtime_index].ring,
                                app_uart_runtimes[runtime_index].ring_storage,
                                sizeof(app_uart_runtimes[runtime_index].ring_storage))
            || !AppUartChunkQueue_Init(&app_uart_runtimes[runtime_index].chunk_queue))
        {
            Error_Handler();
        }
        app_uart_runtimes[runtime_index].next_chunk_silence_before = true;
    }

    app_uart1_rx_semaphore = osSemaphoreNew(32U, 0U, NULL);
    app_bridge_rx_semaphore = osSemaphoreNew(32U, 0U, NULL);
    app_uart_tx_mutex = osMutexNew(NULL);
    app_bridge_mutex = osMutexNew(NULL);
    app_bridge_flags = osEventFlagsNew(NULL);

    App_RuntimeFailFastIfObjectMissing(app_uart1_rx_semaphore);
    App_RuntimeFailFastIfObjectMissing(app_bridge_rx_semaphore);
    App_RuntimeFailFastIfObjectMissing(app_uart_tx_mutex);
    App_RuntimeFailFastIfObjectMissing(app_bridge_mutex);
    App_RuntimeFailFastIfObjectMissing(app_bridge_flags);
}

void App_RuntimeStartUart1Receive(void)
{
    App_RuntimeArmReceive(App_RuntimeFindUartByIndex(1U));
}

void App_RuntimeStartBridgeReceive(void)
{
    App_RuntimeArmReceive(App_RuntimeFindUartByIndex(2U));
    App_RuntimeArmReceive(App_RuntimeFindUartByIndex(3U));
}

void App_RuntimeWaitForUart1Data(void)
{
    (void)osSemaphoreAcquire(app_uart1_rx_semaphore, osWaitForever);
}

void App_RuntimeWaitForBridgeData(void)
{
    (void)osSemaphoreAcquire(app_bridge_rx_semaphore, osWaitForever);
}

bool App_RuntimePopRxByte(uint8_t uart_index, uint8_t *byte)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByIndex(uart_index);

    return (runtime != NULL) && AppRingBuffer_Pop(&runtime->ring, byte);
}

bool App_RuntimePopUart1Chunk(uint8_t *bytes,
                             size_t capacity,
                             size_t *length,
                             bool *silence_before,
                             bool *silence_after)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByIndex(1U);
    AppUartChunk chunk;
    size_t byte_index;

    if ((runtime == NULL) || (bytes == NULL)
        || (capacity < APP_UART_RX_CHUNK_SIZE) || (length == NULL)
        || (silence_before == NULL) || (silence_after == NULL))
    {
        return false;
    }

    if (AppRingBuffer_HasOverflowed(&runtime->ring)
        || AppUartChunkQueue_HasOverflowed(&runtime->chunk_queue))
    {
        return false;
    }

    if (!AppUartChunkQueue_Pop(&runtime->chunk_queue, &chunk))
    {
        return false;
    }

    for (byte_index = 0U; byte_index < chunk.length; ++byte_index)
    {
        if (!AppRingBuffer_Pop(&runtime->ring, &bytes[byte_index]))
        {
            Error_Handler();
            return false;
        }
    }

    *length = chunk.length;
    *silence_before = chunk.silence_before;
    *silence_after = chunk.silence_after;
    return true;
}

bool App_RuntimeConsumeRxOverflow(uint8_t uart_index)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByIndex(uart_index);
    bool has_overflowed;

    if (runtime == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();
    has_overflowed = AppRingBuffer_HasOverflowed(&runtime->ring)
        || AppUartChunkQueue_HasOverflowed(&runtime->chunk_queue);
    if (has_overflowed)
    {
        AppRingBuffer_Reset(&runtime->ring);
        AppUartChunkQueue_Reset(&runtime->chunk_queue);
        runtime->next_chunk_silence_before = true;
    }
    taskEXIT_CRITICAL();

    return has_overflowed;
}

void App_RuntimeFlushRx(uint8_t uart_index)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByIndex(uart_index);

    if (runtime == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();
    AppRingBuffer_Reset(&runtime->ring);
    AppUartChunkQueue_Reset(&runtime->chunk_queue);
    runtime->next_chunk_silence_before = true;
    taskEXIT_CRITICAL();
}

static uint32_t App_RuntimeBridgeMaskForTarget(AppBridgeTarget target)
{
    uint32_t flags = 0U;

    if ((target == APP_BRIDGE_TARGET_UART2) || (target == APP_BRIDGE_TARGET_UART23))
    {
        flags |= APP_BRIDGE_MASK_UART2;
    }
    if ((target == APP_BRIDGE_TARGET_UART3) || (target == APP_BRIDGE_TARGET_UART23))
    {
        flags |= APP_BRIDGE_MASK_UART3;
    }

    return flags;
}

void App_RuntimeSelectBridgeTarget(AppBridgeTarget target)
{
    const uint32_t flags = App_RuntimeBridgeMaskForTarget(target);

    App_RuntimeLockBridge();
    (void)osEventFlagsClear(app_bridge_flags,
                            APP_BRIDGE_MASK_UART2 | APP_BRIDGE_MASK_UART3);
    App_RuntimeFlushRx(2U);
    App_RuntimeFlushRx(3U);
    (void)osEventFlagsSet(app_bridge_flags, flags);
    App_RuntimeUnlockBridge();
}

void App_RuntimeClearBridgeTarget(void)
{
    App_RuntimeLockBridge();

    (void)osEventFlagsClear(app_bridge_flags,
                            APP_BRIDGE_MASK_UART2 | APP_BRIDGE_MASK_UART3);
    App_RuntimeFlushRx(2U);
    App_RuntimeFlushRx(3U);
    App_RuntimeUnlockBridge();
}

uint32_t App_RuntimeGetBridgeMask(void)
{
    return osEventFlagsGet(app_bridge_flags)
        & (APP_BRIDGE_MASK_UART2 | APP_BRIDGE_MASK_UART3);
}

void App_RuntimeLockBridge(void)
{
    if (osMutexAcquire(app_bridge_mutex, osWaitForever) != osOK)
    {
        Error_Handler();
    }
}

void App_RuntimeUnlockBridge(void)
{
    if (osMutexRelease(app_bridge_mutex) != osOK)
    {
        Error_Handler();
    }
}

HAL_StatusTypeDef App_RuntimeSendBytes(UART_HandleTypeDef *uart,
                                       const uint8_t *bytes,
                                       uint16_t length)
{
    HAL_StatusTypeDef status;

    if ((uart == NULL) || (bytes == NULL) || (length == 0U))
    {
        return HAL_ERROR;
    }

    if (osMutexAcquire(app_uart_tx_mutex, osWaitForever) != osOK)
    {
        return HAL_ERROR;
    }

    status = HAL_UART_Transmit(uart,
                              (uint8_t *)bytes,
                              length,
                              APP_UART_TX_TIMEOUT_MS);
    (void)osMutexRelease(app_uart_tx_mutex);
    return status;
}

HAL_StatusTypeDef App_RuntimeSendText(UART_HandleTypeDef *uart, const char *text)
{
    size_t text_length;

    if (text == NULL)
    {
        return HAL_ERROR;
    }

    text_length = strlen(text);
    if ((text_length == 0U) || (text_length > UINT16_MAX))
    {
        return HAL_ERROR;
    }

    return App_RuntimeSendBytes(uart, (const uint8_t *)text, (uint16_t)text_length);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t received_length)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByHandle(uart);
    const bool silence_after = (uart != NULL)
        && (HAL_UARTEx_GetRxEventType(uart) == HAL_UART_RXEVENT_IDLE);
    uint16_t byte_index;

    if (runtime == NULL)
    {
        return;
    }

    for (byte_index = 0U; byte_index < received_length; ++byte_index)
    {
        (void)AppRingBuffer_Push(&runtime->ring, runtime->receive_chunk[byte_index]);
    }

    if (runtime->uart_index == 1U)
    {
        if (received_length > 0U)
        {
            (void)AppUartChunkQueue_Push(&runtime->chunk_queue,
                                         received_length,
                                         runtime->next_chunk_silence_before,
                                         silence_after);
            (void)osSemaphoreRelease(app_uart1_rx_semaphore);
        }
        runtime->next_chunk_silence_before = silence_after;
    }
    else
    {
        (void)osSemaphoreRelease(app_bridge_rx_semaphore);
    }

    App_RuntimeArmReceive(runtime);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    AppUartRuntime *runtime = App_RuntimeFindUartByHandle(uart);

    if (runtime == NULL)
    {
        return;
    }

    (void)HAL_UART_AbortReceive(uart);
    App_RuntimeArmReceive(runtime);
}
