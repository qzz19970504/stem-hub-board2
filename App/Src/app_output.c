#include "app_output.h"

#include "app_config.h"
#include "app_output_math.h"
#include "main.h"
#include "tim.h"

static AppOutputState app_output_state;

static void App_OutputWriteAllState(void)
{
    HAL_GPIO_WritePin(BUCK12V_CTRL_GPIO_Port, BUCK12V_CTRL_Pin,
                      app_output_state.power_12v_enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(BUCK18V_CTRL_GPIO_Port, BUCK18V_CTRL_Pin,
                      app_output_state.power_18v_enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS1_GPIO_Port, NMOS1_Pin,
                      app_output_state.nmos_enabled[0] ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS2_GPIO_Port, NMOS2_Pin,
                      app_output_state.nmos_enabled[1] ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS3_GPIO_Port, NMOS3_Pin,
                      app_output_state.nmos_enabled[2] ? GPIO_PIN_RESET : GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim4,
                          TIM_CHANNEL_4,
                          App_OutputPwmPercentToCompare(app_output_state.pwm_percent,
                                                        __HAL_TIM_GET_AUTORELOAD(&htim4)));
}

void App_OutputInit(void)
{
    AppOutputState_Init(&app_output_state);
    App_OutputWriteAllState();

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

AppOutputResult App_OutputSetNmos(uint8_t nmos_index, bool enabled)
{
    const AppOutputResult result = AppOutputState_SetNmos(&app_output_state,
                                                          nmos_index,
                                                          enabled);
    if (result == APP_OUTPUT_OK)
    {
        App_OutputWriteAllState();
    }
    return result;
}

AppOutputResult App_OutputSetPwmPercent(uint8_t percent)
{
    const AppOutputResult result = AppOutputState_SetPwm(&app_output_state, percent);
    if (result == APP_OUTPUT_OK)
    {
        App_OutputWriteAllState();
    }
    return result;
}

AppOutputResult App_OutputSetPower(AppPowerRail rail, bool enabled)
{
    const AppOutputResult result = AppOutputState_SetPower(&app_output_state, rail, enabled);
    if (result == APP_OUTPUT_OK)
    {
        App_OutputWriteAllState();
    }
    return result;
}

const AppOutputState *App_OutputGetState(void)
{
    return &app_output_state;
}
