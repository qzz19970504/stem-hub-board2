#include "app_output.h"

#include "app_config.h"
#include "app_output_math.h"
#include "main.h"
#include "tim.h"

void App_OutputInit(void)
{
    HAL_GPIO_WritePin(NMOS1_GPIO_Port, NMOS1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS2_GPIO_Port, NMOS2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS3_GPIO_Port, NMOS3_Pin, GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0U);

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

bool App_OutputSetNmos(uint8_t nmos_index, bool enabled)
{
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    const GPIO_PinState pin_state = enabled ? GPIO_PIN_RESET : GPIO_PIN_SET;

    switch (nmos_index)
    {
    case 1U:
        gpio_port = NMOS1_GPIO_Port;
        gpio_pin = NMOS1_Pin;
        break;
    case 2U:
        gpio_port = NMOS2_GPIO_Port;
        gpio_pin = NMOS2_Pin;
        break;
    case 3U:
        gpio_port = NMOS3_GPIO_Port;
        gpio_pin = NMOS3_Pin;
        break;
    default:
        return false;
    }

    HAL_GPIO_WritePin(gpio_port, gpio_pin, pin_state);
    return true;
}

bool App_OutputSetPwmPercent(uint8_t percent)
{
    uint32_t compare;

    if (percent > APP_PWM_MAX_PERCENT)
    {
        return false;
    }

    compare = App_OutputPwmPercentToCompare(
        percent,
        __HAL_TIM_GET_AUTORELOAD(&htim4));
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, compare);
    return true;
}
