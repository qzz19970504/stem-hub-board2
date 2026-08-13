#include "app_output.h"

#include "app_config.h"
#include "app_output_math.h"
#include "app_pwm_fade.h"
#include "app_settings_storage.h"
#include "cmsis_os.h"
#include "main.h"
#include "tim.h"

static AppOutputState app_output_state;
static AppPwmFade app_pwm_fade;
static osMutexId_t app_output_mutex;

static void App_OutputLock(void) { if (osMutexAcquire(app_output_mutex, osWaitForever) != osOK) Error_Handler(); }
static void App_OutputUnlock(void) { if (osMutexRelease(app_output_mutex) != osOK) Error_Handler(); }

static void App_OutputWriteAllState(void)
{
    HAL_GPIO_WritePin(BUCK12V_CTRL_GPIO_Port, BUCK12V_CTRL_Pin,
                      app_output_state.power_12v_enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(BUCK18V_CTRL_GPIO_Port, BUCK18V_CTRL_Pin,
                      app_output_state.power_18v_enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(NMOS1_GPIO_Port, NMOS1_Pin,
                      App_OutputNmosEnabledToPinHigh(app_output_state.nmos_enabled[0])
                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NMOS2_GPIO_Port, NMOS2_Pin,
                      App_OutputNmosEnabledToPinHigh(app_output_state.nmos_enabled[1])
                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NMOS3_GPIO_Port, NMOS3_Pin,
                      App_OutputNmosEnabledToPinHigh(app_output_state.nmos_enabled[2])
                          ? GPIO_PIN_SET : GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim4,
                          TIM_CHANNEL_4,
                          AppPwmFade_GammaCompare(app_output_state.pwm_percent,
                                                  (uint16_t)__HAL_TIM_GET_AUTORELOAD(&htim4)));
}

void App_OutputInit(void)
{
    AppOutputState_Init(&app_output_state);
    app_output_state.pwm_fade_duration_ms = App_SettingsStorageLoadFadeDuration();
    AppPwmFade_Init(&app_pwm_fade, app_output_state.pwm_fade_duration_ms);
    app_output_mutex = osMutexNew(NULL);
    if (app_output_mutex == NULL) Error_Handler();
    App_OutputWriteAllState();

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4) != HAL_OK)
    {
        Error_Handler();
    }
}

AppOutputResult App_OutputSetNmos(uint8_t nmos_index, bool enabled)
{
    App_OutputLock();
    const AppOutputResult result = AppOutputState_SetNmos(&app_output_state,
                                                          nmos_index,
                                                          enabled);
    if (result == APP_OUTPUT_OK)
    {
        App_OutputWriteAllState();
    }
    App_OutputUnlock();
    return result;
}

AppOutputResult App_OutputSetPwmPercent(uint8_t percent)
{
    App_OutputLock();
    if (AppPwmFade_IsBreathing(&app_pwm_fade)) { App_OutputUnlock(); return APP_OUTPUT_DENIED_BREATH; }
    const AppOutputResult result = AppOutputState_SetPwm(&app_output_state, percent);
    if (result == APP_OUTPUT_OK)
    {
        (void)AppPwmFade_SetTarget(&app_pwm_fade, percent);
        app_output_state.pwm_percent = AppPwmFade_GetCurrentPercent(&app_pwm_fade);
        app_output_state.pwm_target_percent = AppPwmFade_GetTargetPercent(&app_pwm_fade);
        App_OutputWriteAllState();
    }
    App_OutputUnlock();
    return result;
}

AppOutputResult App_OutputSetFadeDuration(uint16_t milliseconds)
{
    if (milliseconds > APP_PWM_FADE_MAX_MS) return APP_OUTPUT_INVALID;
    App_OutputLock();
    if (!App_SettingsStorageSaveFadeDuration(milliseconds)) { App_OutputUnlock(); return APP_OUTPUT_STORAGE_ERROR; }
    AppPwmFade_SetDuration(&app_pwm_fade, milliseconds);
    app_output_state.pwm_fade_duration_ms = milliseconds;
    App_OutputUnlock(); return APP_OUTPUT_OK;
}

AppOutputResult App_OutputSetBreathTest(bool enabled)
{
    App_OutputLock();
    if (enabled && !app_output_state.power_18v_enabled) { App_OutputUnlock(); return APP_OUTPUT_DENIED_18V; }
    if (enabled) (void)AppPwmFade_StartBreath(&app_pwm_fade); else AppPwmFade_StopBreath(&app_pwm_fade);
    app_output_state.breath_test_enabled = AppPwmFade_IsBreathing(&app_pwm_fade);
    app_output_state.pwm_target_percent = AppPwmFade_GetTargetPercent(&app_pwm_fade);
    App_OutputUnlock(); return APP_OUTPUT_OK;
}

void App_OutputTick(uint16_t elapsed_ms)
{
    App_OutputLock(); AppPwmFade_Tick(&app_pwm_fade, elapsed_ms);
    app_output_state.pwm_percent = AppPwmFade_GetCurrentPercent(&app_pwm_fade);
    app_output_state.pwm_target_percent = AppPwmFade_GetTargetPercent(&app_pwm_fade);
    app_output_state.breath_test_enabled = AppPwmFade_IsBreathing(&app_pwm_fade);
    App_OutputWriteAllState(); App_OutputUnlock();
}

AppOutputResult App_OutputSetPower(AppPowerRail rail, bool enabled)
{
    App_OutputLock();
    AppOutputResult result;
    if (!enabled && (rail == APP_POWER_RAIL_12V))
    {
        app_output_state.nmos_enabled[0] = false;
        app_output_state.nmos_enabled[1] = false;
        app_output_state.nmos_enabled[2] = false;
        App_OutputWriteAllState();
    }
    else if (!enabled && (rail == APP_POWER_RAIL_18V))
    {
        AppPwmFade_CancelAndClear(&app_pwm_fade);
        app_output_state.pwm_percent = 0U; app_output_state.pwm_target_percent = 0U;
        app_output_state.breath_test_enabled = false;
        App_OutputWriteAllState();
    }
    result = AppOutputState_SetPower(&app_output_state, rail, enabled);
    if (result == APP_OUTPUT_OK)
    {
        App_OutputWriteAllState();
    }
    App_OutputUnlock();
    return result;
}

void App_OutputGetStateSnapshot(AppOutputState *state)
{
    if (state == NULL) return;
    App_OutputLock(); *state = app_output_state; App_OutputUnlock();
}
