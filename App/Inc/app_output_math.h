#ifndef APP_OUTPUT_MATH_H
#define APP_OUTPUT_MATH_H

#include <stdbool.h>
#include <stdint.h>

/** Convert the logical NMOS enable state to its active-high GPIO level. */
bool App_OutputNmosEnabledToPinHigh(bool enabled);

/** Convert an integer duty percent to a timer compare value. */
uint32_t App_OutputPwmPercentToCompare(uint8_t percent, uint32_t auto_reload);

#endif
