#include <assert.h>
#include <string.h>
#include "app_status.h"

int main(void)
{
    AppOutputState state;
    char output[128];
    size_t length = 0U;
    AppOutputState_Init(&state);
    assert(App_StatusEncode(&state, output, sizeof(output), &length));
    assert(strcmp(output, "+STATUS:12V=OFF,18V=OFF,NMOS1=OFF,NMOS2=OFF,NMOS3=OFF,PWM=0,PWM_TARGET=0,PWM_TIME=500,BREATH=OFF\r\nOK\r\n") == 0);
    assert(length == strlen(output));
    state.power_12v_enabled = true;
    state.power_18v_enabled = true;
    state.nmos_enabled[0] = true;
    state.nmos_enabled[2] = true;
    state.pwm_percent = 50U;
    state.pwm_target_percent = 75U;
    state.pwm_fade_duration_ms = 800U;
    state.breath_test_enabled = true;
    assert(App_StatusEncode(&state, output, sizeof(output), &length));
    assert(strcmp(output, "+STATUS:12V=ON,18V=ON,NMOS1=ON,NMOS2=OFF,NMOS3=ON,PWM=50,PWM_TARGET=75,PWM_TIME=800,BREATH=ON\r\nOK\r\n") == 0);
    assert(!App_StatusEncode(&state, output, 8U, &length));
    return 0;
}
