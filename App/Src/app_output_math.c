#include "app_output_math.h"

bool App_OutputNmosEnabledToPinHigh(bool enabled)
{
    return enabled;
}

uint32_t App_OutputPwmPercentToCompare(uint8_t percent, uint32_t auto_reload)
{
    const uint64_t counter_steps = (uint64_t)auto_reload + 1U;

    return (uint32_t)((counter_steps * percent) / 100U);
}
