#ifndef APP_SETTINGS_STORAGE_H
#define APP_SETTINGS_STORAGE_H
#include <stdbool.h>
#include <stdint.h>
uint16_t App_SettingsStorageLoadFadeDuration(void);
bool App_SettingsStorageSaveFadeDuration(uint16_t fade_ms);
#endif
