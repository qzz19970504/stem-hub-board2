#include <assert.h>
#include <stdint.h>

#include "app_ring_buffer.h"

int main(void)
{
    uint8_t storage[4] = {0};
    uint8_t byte = 0U;
    AppRingBuffer ring = {0};

    assert(AppRingBuffer_Init(&ring, storage, sizeof(storage)));
    assert(AppRingBuffer_Count(&ring) == 0U);

    assert(AppRingBuffer_Push(&ring, 1U));
    assert(AppRingBuffer_Push(&ring, 2U));
    assert(AppRingBuffer_Push(&ring, 3U));
    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 1U);
    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 2U);
    assert(AppRingBuffer_Push(&ring, 4U));
    assert(AppRingBuffer_Push(&ring, 5U));
    assert(AppRingBuffer_Push(&ring, 6U));
    assert(AppRingBuffer_Count(&ring) == sizeof(storage));
    assert(!AppRingBuffer_Push(&ring, 7U));
    assert(AppRingBuffer_HasOverflowed(&ring));

    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 3U);
    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 4U);
    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 5U);
    assert(AppRingBuffer_Pop(&ring, &byte) && byte == 6U);
    assert(!AppRingBuffer_Pop(&ring, &byte));

    AppRingBuffer_Reset(&ring);
    assert(AppRingBuffer_Count(&ring) == 0U);
    assert(!AppRingBuffer_HasOverflowed(&ring));
    assert(!AppRingBuffer_Init(NULL, storage, sizeof(storage)));
    assert(!AppRingBuffer_Init(&ring, NULL, sizeof(storage)));
    assert(!AppRingBuffer_Init(&ring, storage, 0U));

    return 0;
}
