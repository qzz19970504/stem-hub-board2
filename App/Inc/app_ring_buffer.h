#ifndef APP_RING_BUFFER_H
#define APP_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *storage;
    size_t capacity;
    volatile size_t head_sequence;
    volatile size_t tail_sequence;
    volatile bool has_overflowed;
} AppRingBuffer;

/** Initialize a single-producer, single-consumer byte ring. */
bool AppRingBuffer_Init(AppRingBuffer *ring, uint8_t *storage, size_t capacity);

/** Push one byte, or mark overflow and return false when full. */
bool AppRingBuffer_Push(AppRingBuffer *ring, uint8_t byte);

/** Pop one byte, returning false when empty. */
bool AppRingBuffer_Pop(AppRingBuffer *ring, uint8_t *byte);

/** Return the number of queued bytes. */
size_t AppRingBuffer_Count(const AppRingBuffer *ring);

/** Return whether a push has failed since initialization or reset. */
bool AppRingBuffer_HasOverflowed(const AppRingBuffer *ring);

/** Discard queued bytes and clear the overflow marker. */
void AppRingBuffer_Reset(AppRingBuffer *ring);

#endif
