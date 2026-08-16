#ifndef APP_OUTPUT_STATE_H
#define APP_OUTPUT_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "app_power.h"

typedef enum
{
    APP_OUTPUT_OK = 0,
    APP_OUTPUT_INVALID,
    APP_OUTPUT_DENIED_12V,
    APP_OUTPUT_DENIED_18V,
    APP_OUTPUT_DENIED_BREATH,
    APP_OUTPUT_STORAGE_ERROR
} AppOutputResult;

typedef struct
{
    bool power_12v_enabled;
    bool power_18v_enabled;
    bool nmos_enabled[3];
    uint8_t pwm_percent;
    uint8_t pwm_target_percent;
    uint16_t pwm_fade_duration_ms;
    bool breath_test_enabled;
} AppOutputState;

void AppOutputState_Init(AppOutputState *state);
AppOutputResult AppOutputState_SetPower(AppOutputState *state,
                                        AppPowerRail rail,
                                        bool enabled);
AppOutputResult AppOutputState_SetNmos(AppOutputState *state,
                                       uint8_t nmos_index,
                                       bool enabled);
AppOutputResult AppOutputState_SetPwm(AppOutputState *state, uint8_t percent);

#endif
