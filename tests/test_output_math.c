#include <assert.h>
#include <stdint.h>

#include "app_output_math.h"

int main(void)
{
    static const uint32_t auto_reload = 999U;

    assert(App_OutputPwmPercentToCompare(0U, auto_reload) == 0U);
    assert(App_OutputPwmPercentToCompare(1U, auto_reload) == 10U);
    assert(App_OutputPwmPercentToCompare(25U, auto_reload) == 250U);
    assert(App_OutputPwmPercentToCompare(50U, auto_reload) == 500U);
    assert(App_OutputPwmPercentToCompare(100U, auto_reload) == 1000U);

    return 0;
}
