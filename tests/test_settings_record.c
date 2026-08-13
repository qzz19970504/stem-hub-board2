#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "app_settings_record.h"

int main(void)
{
    AppSettingsRecord records[4];
    AppSettingsRecord record;
    uint16_t fade_ms = 0U;
    uint32_t sequence = 0U;

    memset(records, 0xFF, sizeof(records));
    assert(!AppSettingsRecord_FindLatest(records, 4U, &fade_ms, &sequence));

    AppSettingsRecord_Create(&record, 500U, 1U);
    assert(AppSettingsRecord_IsValid(&record));
    assert(record.fade_duration_ms == 500U);
    records[1] = record;
    AppSettingsRecord_Create(&records[2], 750U, 2U);
    assert(AppSettingsRecord_FindLatest(records, 4U, &fade_ms, &sequence));
    assert(fade_ms == 750U && sequence == 2U);

    records[2].checksum ^= 1U;
    assert(AppSettingsRecord_FindLatest(records, 4U, &fade_ms, &sequence));
    assert(fade_ms == 500U && sequence == 1U);

    AppSettingsRecord_Create(&records[0], 10001U, 3U);
    assert(!AppSettingsRecord_IsValid(&records[0]));

    memset(records, 0xFF, sizeof(records));
    AppSettingsRecord_Create(&records[0], 100U, UINT32_MAX);
    AppSettingsRecord_Create(&records[1], 200U, 0U);
    assert(AppSettingsRecord_FindLatest(records, 4U, &fade_ms, &sequence));
    assert(fade_ms == 200U && sequence == 0U);
    return 0;
}
