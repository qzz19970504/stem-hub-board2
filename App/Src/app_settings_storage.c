#include "app_settings_storage.h"
#include "app_config.h"
#include "app_settings_record.h"
#include "stm32f1xx_hal.h"

#define SETTINGS_PAGE_ADDRESS 0x0800FC00U
#define SETTINGS_PAGE_SIZE 0x400U
#define SETTINGS_RECORD_COUNT (SETTINGS_PAGE_SIZE / sizeof(AppSettingsRecord))

static const AppSettingsRecord *records(void)
{ return (const AppSettingsRecord *)SETTINGS_PAGE_ADDRESS; }

uint16_t App_SettingsStorageLoadFadeDuration(void)
{
    uint16_t fade_ms=APP_PWM_FADE_DEFAULT_MS; uint32_t sequence;
    (void)AppSettingsRecord_FindLatest(records(),SETTINGS_RECORD_COUNT,&fade_ms,&sequence);
    return fade_ms;
}

bool App_SettingsStorageSaveFadeDuration(uint16_t fade_ms)
{
    uint16_t current; uint32_t sequence=0U; size_t index; AppSettingsRecord record;
    const bool found=AppSettingsRecord_FindLatest(records(),SETTINGS_RECORD_COUNT,&current,&sequence);
    (void)current;
    for(index=0U;index<SETTINGS_RECORD_COUNT;++index) if(records()[index].magic==UINT32_MAX) break;
    if(HAL_FLASH_Unlock()!=HAL_OK) return false;
    if(index==SETTINGS_RECORD_COUNT) {
        FLASH_EraseInitTypeDef erase={.TypeErase=FLASH_TYPEERASE_PAGES,.PageAddress=SETTINGS_PAGE_ADDRESS,.NbPages=1U}; uint32_t error;
        if(HAL_FLASHEx_Erase(&erase,&error)!=HAL_OK){(void)HAL_FLASH_Lock();return false;} index=0U;
    }
    AppSettingsRecord_Create(&record,fade_ms,found?sequence+1U:0U);
    for(size_t word=0U;word<sizeof(record)/4U;++word) if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,SETTINGS_PAGE_ADDRESS+index*sizeof(record)+word*4U,((const uint32_t *)&record)[word])!=HAL_OK){(void)HAL_FLASH_Lock();return false;}
    (void)HAL_FLASH_Lock();
    return AppSettingsRecord_IsValid(&records()[index]);
}
