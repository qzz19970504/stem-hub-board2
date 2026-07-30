#ifndef APP_OUTPUT_H
#define APP_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

/** Put all switched outputs into their safe state and start PWM at 0%. */
void App_OutputInit(void);

/** Set one active-low NMOS output. */
bool App_OutputSetNmos(uint8_t nmos_index, bool enabled);

/** Set PWM_LED to an integer duty percent from 0 through 100. */
bool App_OutputSetPwmPercent(uint8_t percent);

#endif
