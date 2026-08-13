#include <assert.h>
#include "app_output_state.h"

int main(void)
{
    AppOutputState state;
    AppOutputState_Init(&state);
    assert(!state.power_12v_enabled && !state.power_18v_enabled);
    assert(!state.nmos_enabled[0] && !state.nmos_enabled[1] && !state.nmos_enabled[2]);
    assert(state.pwm_percent == 0U);

    assert(AppOutputState_SetNmos(&state, 1U, true) == APP_OUTPUT_DENIED_12V);
    assert(AppOutputState_SetPwm(&state, 25U) == APP_OUTPUT_DENIED_18V);
    assert(AppOutputState_SetPower(&state, APP_POWER_RAIL_12V, true) == APP_OUTPUT_OK);
    assert(AppOutputState_SetNmos(&state, 1U, true) == APP_OUTPUT_OK);
    assert(state.nmos_enabled[0]);
    assert(AppOutputState_SetPower(&state, APP_POWER_RAIL_12V, false) == APP_OUTPUT_OK);
    assert(!state.nmos_enabled[0]);
    assert(AppOutputState_SetNmos(&state, 1U, false) == APP_OUTPUT_OK);

    assert(AppOutputState_SetPower(&state, APP_POWER_RAIL_18V, true) == APP_OUTPUT_OK);
    assert(AppOutputState_SetPwm(&state, 75U) == APP_OUTPUT_OK);
    assert(state.pwm_percent == 75U);
    assert(AppOutputState_SetPower(&state, APP_POWER_RAIL_18V, false) == APP_OUTPUT_OK);
    assert(state.pwm_percent == 0U);
    assert(AppOutputState_SetPwm(&state, 0U) == APP_OUTPUT_OK);
    assert(AppOutputState_SetPwm(&state, 101U) == APP_OUTPUT_INVALID);
    return 0;
}
