#include "app_settings_record.h"
#include "app_config.h"

static uint32_t checksum(const AppSettingsRecord *record)
{
    return record->magic ^ record->sequence ^ (uint32_t)record->fade_duration_ms
        ^ ((uint32_t)record->version << 16U) ^ 0xA55A3CC3U;
}
void AppSettingsRecord_Create(AppSettingsRecord *record, uint16_t fade_ms, uint32_t sequence)
{
    record->magic=APP_SETTINGS_RECORD_MAGIC;
    record->sequence=sequence;
    record->fade_duration_ms=fade_ms;
    record->version=1U;
    record->checksum=checksum(record);
}
bool AppSettingsRecord_IsValid(const AppSettingsRecord *record)
{
    return record && record->magic==APP_SETTINGS_RECORD_MAGIC && record->version==1U
        && record->fade_duration_ms<=APP_PWM_FADE_MAX_MS && record->checksum==checksum(record);
}
static bool newer(uint32_t candidate, uint32_t current)
{ return (int32_t)(candidate-current)>0; }
bool AppSettingsRecord_FindLatest(const AppSettingsRecord *records, size_t count, uint16_t *fade_ms, uint32_t *sequence)
{
    size_t index; bool found=false; uint32_t latest=0U;
    if (!records || !fade_ms || !sequence) return false;
    for(index=0U;index<count;++index) if(AppSettingsRecord_IsValid(&records[index]) && (!found || newer(records[index].sequence,latest))) {
        found=true; latest=records[index].sequence; *fade_ms=records[index].fade_duration_ms;
    }
    if(found)*sequence=latest;
    return found;
}
