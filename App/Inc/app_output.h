#ifndef APP_OUTPUT_H
#define APP_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "app_output_state.h"

/** Put all switched outputs into their safe state and start PWM at 0%. */
void App_OutputInit(void);

/** Set one active-low NMOS output. */
AppOutputResult App_OutputSetNmos(uint8_t nmos_index, bool enabled);

/** Set PWM_LED to an integer duty percent from 0 through 100. */
AppOutputResult App_OutputSetPwmPercent(uint8_t percent);

AppOutputResult App_OutputSetPower(AppPowerRail rail, bool enabled);
const AppOutputState *App_OutputGetState(void);

#endif
