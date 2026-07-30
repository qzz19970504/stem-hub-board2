#include "app_ring_buffer.h"

bool AppRingBuffer_Init(AppRingBuffer *ring, uint8_t *storage, size_t capacity)
{
    if ((ring == NULL) || (storage == NULL) || (capacity == 0U))
    {
        return false;
    }

    ring->storage = storage;
    ring->capacity = capacity;
    ring->head_sequence = 0U;
    ring->tail_sequence = 0U;
    ring->has_overflowed = false;
    return true;
}

bool AppRingBuffer_Push(AppRingBuffer *ring, uint8_t byte)
{
    size_t head_sequence;

    if ((ring == NULL) || (ring->storage == NULL) || (ring->capacity == 0U))
    {
        return false;
    }

    head_sequence = ring->head_sequence;
    if ((head_sequence - ring->tail_sequence) >= ring->capacity)
    {
        ring->has_overflowed = true;
        return false;
    }

    ring->storage[head_sequence % ring->capacity] = byte;
    ring->head_sequence = head_sequence + 1U;
    return true;
}

bool AppRingBuffer_Pop(AppRingBuffer *ring, uint8_t *byte)
{
    size_t tail_sequence;

    if ((ring == NULL) || (byte == NULL) || (ring->storage == NULL))
    {
        return false;
    }

    tail_sequence = ring->tail_sequence;
    if (tail_sequence == ring->head_sequence)
    {
        return false;
    }

    *byte = ring->storage[tail_sequence % ring->capacity];
    ring->tail_sequence = tail_sequence + 1U;
    return true;
}

size_t AppRingBuffer_Count(const AppRingBuffer *ring)
{
    if (ring == NULL)
    {
        return 0U;
    }

    return ring->head_sequence - ring->tail_sequence;
}

bool AppRingBuffer_HasOverflowed(const AppRingBuffer *ring)
{
    return (ring != NULL) && ring->has_overflowed;
}

void AppRingBuffer_Reset(AppRingBuffer *ring)
{
    if (ring == NULL)
    {
        return;
    }

    ring->tail_sequence = ring->head_sequence;
    ring->has_overflowed = false;
}
