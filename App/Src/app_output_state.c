#include "app_output_state.h"

#include <string.h>
#include "app_config.h"

void AppOutputState_Init(AppOutputState *state)
{
    if (state != NULL)
    {
        (void)memset(state, 0, sizeof(*state));
        state->pwm_fade_duration_ms = APP_PWM_FADE_DEFAULT_MS;
    }
}

AppOutputResult AppOutputState_SetPower(AppOutputState *state,
                                        AppPowerRail rail,
                                        bool enabled)
{
    if (state == NULL)
    {
        return APP_OUTPUT_INVALID;
    }
    if (rail == APP_POWER_RAIL_12V)
    {
        state->power_12v_enabled = enabled;
        if (!enabled)
        {
            state->nmos_enabled[0] = false;
            state->nmos_enabled[1] = false;
            state->nmos_enabled[2] = false;
        }
        return APP_OUTPUT_OK;
    }
    if (rail == APP_POWER_RAIL_18V)
    {
        state->power_18v_enabled = enabled;
        if (!enabled)
        {
            state->pwm_percent = 0U;
            state->pwm_target_percent = 0U;
            state->breath_test_enabled = false;
        }
        return APP_OUTPUT_OK;
    }
    return APP_OUTPUT_INVALID;
}

AppOutputResult AppOutputState_SetNmos(AppOutputState *state,
                                       uint8_t nmos_index,
                                       bool enabled)
{
    if ((state == NULL) || (nmos_index < 1U) || (nmos_index > 3U))
    {
        return APP_OUTPUT_INVALID;
    }
    if (enabled && !state->power_12v_enabled)
    {
        return APP_OUTPUT_DENIED_12V;
    }
    state->nmos_enabled[nmos_index - 1U] = enabled;
    return APP_OUTPUT_OK;
}

AppOutputResult AppOutputState_SetPwm(AppOutputState *state, uint8_t percent)
{
    if ((state == NULL) || (percent > APP_PWM_MAX_PERCENT))
    {
        return APP_OUTPUT_INVALID;
    }
    if ((percent > 0U) && !state->power_18v_enabled)
    {
        return APP_OUTPUT_DENIED_18V;
    }
    state->pwm_target_percent = percent;
    return APP_OUTPUT_OK;
}
