#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_uart_chunk_queue.h"

int main(void)
{
    AppUartChunkQueue queue;
    AppUartChunk chunk;
    size_t index;

    assert(AppUartChunkQueue_Init(&queue));
    assert(!AppUartChunkQueue_Pop(&queue, &chunk));
    assert(!AppUartChunkQueue_HasOverflowed(&queue));

    assert(AppUartChunkQueue_Push(&queue, 3U, true, true));
    assert(AppUartChunkQueue_Pop(&queue, &chunk));
    assert(chunk.length == 3U);
    assert(chunk.silence_before);
    assert(chunk.silence_after);

    for (index = 0U; index < APP_UART_CHUNK_QUEUE_CAPACITY; ++index)
    {
        assert(AppUartChunkQueue_Push(&queue,
                                      (uint16_t)(index + 1U),
                                      (index % 2U) == 0U,
                                      (index % 3U) == 0U));
    }
    assert(!AppUartChunkQueue_Push(&queue, 99U, false, false));
    assert(AppUartChunkQueue_HasOverflowed(&queue));

    for (index = 0U; index < APP_UART_CHUNK_QUEUE_CAPACITY; ++index)
    {
        assert(AppUartChunkQueue_Pop(&queue, &chunk));
        assert(chunk.length == (uint16_t)(index + 1U));
        assert(chunk.silence_before == ((index % 2U) == 0U));
        assert(chunk.silence_after == ((index % 3U) == 0U));
    }
    assert(!AppUartChunkQueue_Pop(&queue, &chunk));

    AppUartChunkQueue_Reset(&queue);
    assert(!AppUartChunkQueue_HasOverflowed(&queue));
    assert(!AppUartChunkQueue_Pop(&queue, &chunk));

    for (index = 0U; index < (APP_UART_CHUNK_QUEUE_CAPACITY / 2U); ++index)
    {
        assert(AppUartChunkQueue_Push(&queue, (uint16_t)(10U + index), true, false));
    }
    for (index = 0U; index < (APP_UART_CHUNK_QUEUE_CAPACITY / 2U); ++index)
    {
        assert(AppUartChunkQueue_Pop(&queue, &chunk));
        assert(chunk.length == (uint16_t)(10U + index));
    }
    for (index = 0U; index < APP_UART_CHUNK_QUEUE_CAPACITY; ++index)
    {
        assert(AppUartChunkQueue_Push(&queue, (uint16_t)(20U + index), false, true));
    }
    for (index = 0U; index < APP_UART_CHUNK_QUEUE_CAPACITY; ++index)
    {
        assert(AppUartChunkQueue_Pop(&queue, &chunk));
        assert(chunk.length == (uint16_t)(20U + index));
        assert(!chunk.silence_before);
        assert(chunk.silence_after);
    }

    assert(!AppUartChunkQueue_Init(NULL));
    assert(!AppUartChunkQueue_Push(NULL, 1U, false, false));
    assert(!AppUartChunkQueue_Push(&queue, 0U, false, false));
    assert(!AppUartChunkQueue_Pop(NULL, &chunk));
    assert(!AppUartChunkQueue_Pop(&queue, NULL));
    assert(!AppUartChunkQueue_HasOverflowed(NULL));

    return 0;
}
