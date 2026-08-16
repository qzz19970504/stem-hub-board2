#include "app_pwm_fade.h"
#include <string.h>

static const uint16_t gamma22[] = {
0,3,12,29,55,90,134,189,253,328,413,510,618,736,867,1009,1163,1329,1507,1697,1900,2115,2343,2584,2838,3104,3384,3677,3983,4303,4636,4983,5343,5717,6106,6508,6924,7354,7798,8257,8730,9217,9719,10235,10766,11312,11872,12448,13038,13643,14263,14898,15548,16214,16894,17590,18302,19028,19770,20528,21301,22090,22895,23715,24551,25403,26271,27154,28054,28970,29901,30849,31813,32793,33790,34802,35831,36877,37939,39017,40112,41223,42351,43496,44657,45835,47029,48241,49469,50714,51976,53255,54551,55864,57195,58542,59906,61287,62686,64102,65535};

static void begin(AppPwmFade *fade, uint8_t target)
{
    fade->start_percent = fade->current_percent;
    fade->target_percent = target;
    fade->segment_duration_ms = fade->duration_ms;
    fade->elapsed_ms = 0U;
    if (fade->segment_duration_ms == 0U) fade->current_percent = target;
}
static void begin_breath_segment(AppPwmFade *fade, uint8_t target)
{
    begin(fade, target);
    if (fade->segment_duration_ms < 100U) fade->segment_duration_ms = 100U;
}
void AppPwmFade_Init(AppPwmFade *fade, uint16_t duration_ms)
{ memset(fade, 0, sizeof(*fade)); fade->duration_ms = duration_ms; }
void AppPwmFade_SetDuration(AppPwmFade *fade, uint16_t duration_ms) { fade->duration_ms = duration_ms; }
bool AppPwmFade_SetTarget(AppPwmFade *fade, uint8_t percent)
{ if (fade->breathing || percent > 100U) return false; begin(fade, percent); return true; }
bool AppPwmFade_StartBreath(AppPwmFade *fade)
{ if (fade->breathing) return true; fade->saved_target_percent=fade->target_percent; fade->breathing=true; begin_breath_segment(fade,100U); return true; }
void AppPwmFade_StopBreath(AppPwmFade *fade)
{ if (fade->breathing) { fade->breathing=false; begin(fade,fade->saved_target_percent); } }
void AppPwmFade_CancelAndClear(AppPwmFade *fade)
{ fade->breathing=false; fade->current_percent=0U; fade->target_percent=0U; fade->start_percent=0U; fade->elapsed_ms=0U; }
void AppPwmFade_Tick(AppPwmFade *fade, uint16_t elapsed_ms)
{
    int32_t delta;
    if (fade->current_percent == fade->target_percent) {
        if (fade->breathing) begin_breath_segment(fade, fade->target_percent == 100U ? 0U : 100U);
        else return;
    }
    if (fade->segment_duration_ms == 0U) fade->current_percent=fade->target_percent;
    else {
        uint32_t next=(uint32_t)fade->elapsed_ms+elapsed_ms;
        fade->elapsed_ms=(uint16_t)(next>fade->segment_duration_ms?fade->segment_duration_ms:next);
        delta=(int32_t)fade->target_percent-(int32_t)fade->start_percent;
        fade->current_percent=(uint8_t)((int32_t)fade->start_percent + delta*(int32_t)fade->elapsed_ms/(int32_t)fade->segment_duration_ms);
    }
}
uint8_t AppPwmFade_GetCurrentPercent(const AppPwmFade *fade){return fade->current_percent;}
uint8_t AppPwmFade_GetTargetPercent(const AppPwmFade *fade){return fade->target_percent;}
uint16_t AppPwmFade_GetDuration(const AppPwmFade *fade){return fade->duration_ms;}
bool AppPwmFade_IsBreathing(const AppPwmFade *fade){return fade->breathing;}
uint16_t AppPwmFade_GammaCompare(uint8_t percent, uint16_t auto_reload)
{
    const uint32_t steps=(uint32_t)auto_reload+1U;
    if (percent > 100U) percent = 100U;
    return (uint16_t)(((uint32_t)gamma22[percent]*steps+32767U)/65535U);
}
