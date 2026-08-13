#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "app_pwm_fade.h"

int main(void)
{
    AppPwmFade fade;
    uint16_t previous_compare = 0U;
    uint8_t percent;

    AppPwmFade_Init(&fade, 100U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 0U);
    assert(AppPwmFade_SetTarget(&fade, 60U));
    for (uint8_t tick = 0U; tick < 5U; ++tick)
    {
        AppPwmFade_Tick(&fade, 10U);
    }
    assert(AppPwmFade_GetCurrentPercent(&fade) == 30U);
    for (uint8_t tick = 0U; tick < 5U; ++tick)
    {
        AppPwmFade_Tick(&fade, 10U);
    }
    assert(AppPwmFade_GetCurrentPercent(&fade) == 60U);

    assert(AppPwmFade_SetTarget(&fade, 0U));
    AppPwmFade_Tick(&fade, 50U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 30U);
    assert(AppPwmFade_SetTarget(&fade, 80U));
    AppPwmFade_Tick(&fade, 50U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 55U);
    AppPwmFade_Tick(&fade, 50U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 80U);

    AppPwmFade_SetDuration(&fade, 0U);
    assert(AppPwmFade_SetTarget(&fade, 20U));
    assert(AppPwmFade_GetCurrentPercent(&fade) == 20U);

    for (percent = 0U; percent <= 100U; ++percent)
    {
        const uint16_t compare = AppPwmFade_GammaCompare(percent, 2559U);
        assert(compare >= previous_compare);
        previous_compare = compare;
    }
    assert(AppPwmFade_GammaCompare(0U, 2559U) == 0U);
    assert(AppPwmFade_GammaCompare(100U, 2559U) == 2560U);
    assert(AppPwmFade_GammaCompare(50U, 2559U) < 1280U);

    AppPwmFade_SetDuration(&fade, 100U);
    assert(AppPwmFade_SetTarget(&fade, 40U));
    AppPwmFade_Tick(&fade, 100U);
    assert(AppPwmFade_StartBreath(&fade));
    assert(AppPwmFade_IsBreathing(&fade));
    assert(!AppPwmFade_SetTarget(&fade, 10U));
    AppPwmFade_Tick(&fade, 100U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 100U);
    AppPwmFade_Tick(&fade, 100U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 0U);
    AppPwmFade_StopBreath(&fade);
    assert(!AppPwmFade_IsBreathing(&fade));
    AppPwmFade_Tick(&fade, 100U);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 40U);

    AppPwmFade_CancelAndClear(&fade);
    assert(AppPwmFade_GetCurrentPercent(&fade) == 0U);
    assert(AppPwmFade_GetTargetPercent(&fade) == 0U);
    assert(!AppPwmFade_IsBreathing(&fade));
    return 0;
}
