#include "app_status.h"

#include <stdio.h>

bool App_StatusEncode(const AppOutputState *state,
                      char *output,
                      size_t output_capacity,
                      size_t *output_length)
{
    int written;
    if ((state == NULL) || (output == NULL) || (output_length == NULL))
    {
        return false;
    }
    written = snprintf(output,
                       output_capacity,
                       "+STATUS:12V=%s,18V=%s,NMOS1=%s,NMOS2=%s,NMOS3=%s,PWM=%u\r\nOK\r\n",
                       state->power_12v_enabled ? "ON" : "OFF",
                       state->power_18v_enabled ? "ON" : "OFF",
                       state->nmos_enabled[0] ? "ON" : "OFF",
                       state->nmos_enabled[1] ? "ON" : "OFF",
                       state->nmos_enabled[2] ? "ON" : "OFF",
                       (unsigned int)state->pwm_percent);
    if ((written < 0) || ((size_t)written >= output_capacity))
    {
        return false;
    }
    *output_length = (size_t)written;
    return true;
}
