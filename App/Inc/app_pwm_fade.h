#ifndef APP_PWM_FADE_H
#define APP_PWM_FADE_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint8_t current_percent, target_percent, start_percent, saved_target_percent;
    uint16_t duration_ms, segment_duration_ms, elapsed_ms;
    bool breathing;
} AppPwmFade;
void AppPwmFade_Init(AppPwmFade *fade, uint16_t duration_ms);
void AppPwmFade_SetDuration(AppPwmFade *fade, uint16_t duration_ms);
bool AppPwmFade_SetTarget(AppPwmFade *fade, uint8_t percent);
bool AppPwmFade_StartBreath(AppPwmFade *fade);
void AppPwmFade_StopBreath(AppPwmFade *fade);
void AppPwmFade_CancelAndClear(AppPwmFade *fade);
void AppPwmFade_Tick(AppPwmFade *fade, uint16_t elapsed_ms);
uint8_t AppPwmFade_GetCurrentPercent(const AppPwmFade *fade);
uint8_t AppPwmFade_GetTargetPercent(const AppPwmFade *fade);
uint16_t AppPwmFade_GetDuration(const AppPwmFade *fade);
bool AppPwmFade_IsBreathing(const AppPwmFade *fade);
uint16_t AppPwmFade_GammaCompare(uint8_t percent, uint16_t auto_reload);
#endif
