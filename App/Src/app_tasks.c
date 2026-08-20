#include "app_tasks.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_at_protocol.h"
#include "app_config.h"
#include "app_line_reader.h"
#include "app_output.h"
#include "app_runtime.h"
#include "app_uart_tunnel.h"
#include "app_status.h"
#include "app_transparent_mode.h"
#include "cmsis_os.h"
#include "main.h"
#include "usart.h"

static void App_AtSendParseError(AppAtParseStatus parse_status)
{
    const char *error = (parse_status == APP_AT_PARSE_RANGE)
        ? "+ERROR:RANGE\r\n"
        : "+ERROR:PARSE\r\n";

    (void)App_RuntimeSendText(&huart1, error);
}

static void App_AtForwardBytes(const uint8_t *bytes, size_t length)
{
    const uint32_t bridge_mask = App_RuntimeGetBridgeMask();

    if ((bytes == NULL) || (length == 0U))
    {
        return;
    }

    if ((bridge_mask & APP_BRIDGE_MASK_UART2) != 0U)
    {
        (void)App_RuntimeSendBytes(&huart2, bytes, (uint16_t)length);
    }
    if ((bridge_mask & APP_BRIDGE_MASK_UART3) != 0U)
    {
        (void)App_RuntimeSendBytes(&huart3, bytes, (uint16_t)length);
    }
}

static void App_AtSendUartPayload(const AppAtUartPayloadCommand *payload)
{
    const uint32_t bridge_mask = App_RuntimeGetBridgeMask();
    HAL_StatusTypeDef uart2_status = HAL_OK;
    HAL_StatusTypeDef uart3_status = HAL_OK;

    if (bridge_mask == 0U)
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:UART_DISABLED\r\n");
        return;
    }

    if ((bridge_mask & APP_BRIDGE_MASK_UART2) != 0U)
    {
        uart2_status = App_RuntimeSendBytes(
            &huart2,
            payload->bytes,
            (uint16_t)payload->length);
    }
    if ((bridge_mask & APP_BRIDGE_MASK_UART3) != 0U)
    {
        uart3_status = App_RuntimeSendBytes(
            &huart3,
            payload->bytes,
            (uint16_t)payload->length);
    }

    if ((uart2_status == HAL_OK) && (uart3_status == HAL_OK))
    {
        (void)App_RuntimeSendText(&huart1, "OK\r\n");
    }
    else
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:UART_TX\r\n");
    }
}

static void App_AtHandleCommand(const AppAtCommand *command,
                                AppTransparentMode *transparent_mode)
{
    AppOutputResult output_result = APP_OUTPUT_INVALID;
    bool success = false;

    switch (command->type)
    {
    case APP_AT_COMMAND_SET_NMOS:
        output_result = App_OutputSetNmos(command->data.nmos.index,
                                          command->data.nmos.enabled);
        break;
    case APP_AT_COMMAND_SET_PWM:
        output_result = App_OutputSetPwmPercent(command->data.pwm.percent);
        break;
    case APP_AT_COMMAND_SET_PWM_TIME:
        output_result = App_OutputSetFadeDuration(command->data.pwm_time.milliseconds);
        break;
    case APP_AT_COMMAND_SET_BREATH_TEST:
        output_result = App_OutputSetBreathTest(command->data.breath.enabled);
        break;
    case APP_AT_COMMAND_SET_POWER:
        output_result = App_OutputSetPower(command->data.power.rail,
                                           command->data.power.enabled);
        break;
    case APP_AT_COMMAND_GET_STATUS:
    {
        char status[APP_AT_PROTOCOL_MAX_LINE_LENGTH];
        size_t status_length = 0U;
        AppOutputState state;
        App_OutputGetStateSnapshot(&state);
        if (App_StatusEncode(&state, status, sizeof(status), &status_length))
        {
            (void)App_RuntimeSendBytes(&huart1,
                                       (const uint8_t *)status,
                                       (uint16_t)status_length);
        }
        else
        {
            (void)App_RuntimeSendText(&huart1, "+ERROR:PARSE\r\n");
        }
        return;
    }
    case APP_AT_COMMAND_START_TRANSPARENT:
        (void)App_RuntimeSendText(&huart1, "OK\r\n");
        App_RuntimeSelectBridgeTarget(command->data.transparent.target);
        AppTransparentMode_Enter(transparent_mode,
                                 command->data.transparent.target);
        return;
    case APP_AT_COMMAND_SEND_UART:
        App_AtSendUartPayload(&command->data.uart_payload);
        return;
    default:
        break;
    }

    if (output_result == APP_OUTPUT_DENIED_12V)
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:12V_DISABLED\r\n");
        return;
    }
    if (output_result == APP_OUTPUT_DENIED_18V)
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:18V_DISABLED\r\n");
        return;
    }
    if (output_result == APP_OUTPUT_DENIED_BREATH)
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:BREATH_ACTIVE\r\n");
        return;
    }
    if (output_result == APP_OUTPUT_STORAGE_ERROR)
    {
        (void)App_RuntimeSendText(&huart1, "+ERROR:STORAGE\r\n");
        return;
    }
    if (output_result == APP_OUTPUT_OK)
    {
        success = true;
    }

    (void)App_RuntimeSendText(
        &huart1,
        success ? "OK\r\n" : "+ERROR:PARSE\r\n");
}

static void App_AtProcessFrame(const uint8_t *frame,
                               size_t frame_length,
                               AppTransparentMode *transparent_mode)
{
    AppAtCommand command = {0};
    const AppAtParseStatus parse_status =
        AppAtProtocol_ParseFrame(frame, frame_length, &command);

    if (parse_status == APP_AT_PARSE_NOT_AT)
    {
        return;
    }

    if (parse_status != APP_AT_PARSE_OK)
    {
        App_AtSendParseError(parse_status);
        return;
    }

    App_AtHandleCommand(&command, transparent_mode);
}

static void App_AtProcessTransparentChunk(AppTransparentMode *transparent_mode,
                                          const uint8_t *bytes,
                                          size_t length,
                                          bool silence_before,
                                          bool silence_after)
{
    AppTransparentResult result;

    if (!AppTransparentMode_ProcessChunk(transparent_mode,
                                         bytes,
                                         length,
                                         silence_before,
                                         silence_after,
                                         &result))
    {
        return;
    }

    App_AtForwardBytes(result.forward, result.forward_length);
    if (result.exited)
    {
        App_RuntimeClearBridgeTarget();
        (void)App_RuntimeSendText(&huart1, "OK\r\n");
    }
}

static void App_AtConsumeBytes(AppLineReader *line_reader,
                               AppTransparentMode *transparent_mode,
                               const uint8_t *bytes,
                               size_t length,
                               bool silence_after)
{
    size_t byte_index;

    for (byte_index = 0U; byte_index < length; ++byte_index)
    {
        const AppLineReaderStatus line_status =
            AppLineReader_Push(line_reader, bytes[byte_index]);

        if (line_status == APP_LINE_READER_TOO_LONG)
        {
            (void)App_RuntimeSendText(&huart1, "+ERROR:LINE_TOO_LONG\r\n");
            continue;
        }

        if (line_status == APP_LINE_READER_COMPLETE)
        {
            App_AtProcessFrame(
                (const uint8_t *)AppLineReader_GetLine(line_reader),
                AppLineReader_GetLineLength(line_reader),
                transparent_mode);
            AppLineReader_Reset(line_reader);

            if (AppTransparentMode_IsActive(transparent_mode)
                && ((byte_index + 1U) < length))
            {
                App_AtProcessTransparentChunk(transparent_mode,
                                              &bytes[byte_index + 1U],
                                              length - byte_index - 1U,
                                              false,
                                              silence_after);
                return;
            }
        }
    }
}

void App_SystemTask(void *argument)
{
    (void)argument;

    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

    for (;;)
    {
        App_OutputTick(APP_PWM_FADE_TICK_MS);
        osDelay(APP_PWM_FADE_TICK_MS);
    }
}

void App_AtTask(void *argument)
{
    static AppTransparentMode transparent_mode;
    static uint8_t chunk[APP_UART_RX_CHUNK_SIZE];
    char line_buffer[APP_AT_PROTOCOL_MAX_LINE_LENGTH] = {0};
    AppLineReader line_reader = {0};
    size_t chunk_length;
    bool silence_before;
    bool silence_after;

    (void)argument;
    if (!AppLineReader_Init(&line_reader, line_buffer, sizeof(line_buffer)))
    {
        Error_Handler();
    }
    AppTransparentMode_Init(&transparent_mode);
    App_RuntimeStartUart1Receive();

    for (;;)
    {
        App_RuntimeWaitForUart1Data();

        if (App_RuntimeConsumeRxOverflow(1U))
        {
            if (AppTransparentMode_IsActive(&transparent_mode))
            {
                AppTransparentMode_Abort(&transparent_mode);
                App_RuntimeClearBridgeTarget();
            }
            AppLineReader_Reset(&line_reader);
            (void)App_RuntimeSendText(&huart1, "+ERROR:RX_OVERFLOW\r\n");
            continue;
        }

        while (App_RuntimePopUart1Chunk(chunk,
                                       sizeof(chunk),
                                       &chunk_length,
                                       &silence_before,
                                       &silence_after))
        {
            if (AppTransparentMode_IsActive(&transparent_mode))
            {
                App_AtProcessTransparentChunk(&transparent_mode,
                                              chunk,
                                              chunk_length,
                                              silence_before,
                                              silence_after);
                continue;
            }

            App_AtConsumeBytes(&line_reader,
                               &transparent_mode,
                               chunk,
                               chunk_length,
                               silence_after);
        }
    }
}

static void App_BridgeDrainUart(uint8_t uart_index, uint32_t enable_mask)
{
    uint8_t payload[APP_UART_TUNNEL_MAX_PAYLOAD_SIZE];
    size_t payload_length;
    char event[APP_AT_PROTOCOL_MAX_LINE_LENGTH];
    size_t event_length;
    uint8_t byte;

    do
    {
        App_RuntimeLockBridge();
        if ((App_RuntimeGetBridgeMask() & enable_mask) == 0U)
        {
            App_RuntimeFlushRx(uart_index);
            App_RuntimeUnlockBridge();
            return;
        }

        if (App_RuntimeConsumeRxOverflow(uart_index))
        {
            (void)App_RuntimeSendText(&huart1, "+ERROR:RX_OVERFLOW\r\n");
            App_RuntimeUnlockBridge();
            return;
        }

        payload_length = 0U;
        while ((payload_length < sizeof(payload))
               && App_RuntimePopRxByte(uart_index, &byte))
        {
            payload[payload_length++] = byte;
        }

        if ((payload_length > 0U)
            && App_UartTunnelEncodeEvent(uart_index,
                                         payload,
                                         payload_length,
                                         event,
                                         sizeof(event),
                                         &event_length))
        {
            (void)App_RuntimeSendBytes(
                &huart1,
                (const uint8_t *)event,
                (uint16_t)event_length);
        }
        App_RuntimeUnlockBridge();
    } while (payload_length == sizeof(payload));
}

void App_BridgeTask(void *argument)
{
    (void)argument;
    App_RuntimeStartBridgeReceive();

    for (;;)
    {
        App_RuntimeWaitForBridgeData();
        App_BridgeDrainUart(2U, APP_BRIDGE_MASK_UART2);
        App_BridgeDrainUart(3U, APP_BRIDGE_MASK_UART3);
    }
}
