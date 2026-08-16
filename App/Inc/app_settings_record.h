#ifndef APP_SETTINGS_RECORD_H
#define APP_SETTINGS_RECORD_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define APP_SETTINGS_RECORD_MAGIC 0x5046574DU
typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint16_t fade_duration_ms;
    uint16_t version;
    uint32_t checksum;
} AppSettingsRecord;
void AppSettingsRecord_Create(AppSettingsRecord *record, uint16_t fade_ms, uint32_t sequence);
bool AppSettingsRecord_IsValid(const AppSettingsRecord *record);
bool AppSettingsRecord_FindLatest(const AppSettingsRecord *records, size_t count, uint16_t *fade_ms, uint32_t *sequence);
#endif
