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
#include "cmsis_os.h"
#include "main.h"
#include "usart.h"

#define APP_SYSTEM_HEARTBEAT_DELAY_MS 1000U

static void App_AtSendParseError(AppAtParseStatus parse_status)
{
    const char *error = (parse_status == APP_AT_PARSE_RANGE)
        ? "+ERROR:RANGE\r\n"
        : "+ERROR:PARSE\r\n";

    (void)App_RuntimeSendText(&huart1, error);
}

static void App_AtForwardFrame(const uint8_t *frame, size_t frame_length)
{
    const uint32_t bridge_mask = App_RuntimeGetBridgeMask();

    if ((bridge_mask & APP_BRIDGE_MASK_UART2) != 0U)
    {
        (void)App_RuntimeSendBytes(&huart2, frame, (uint16_t)frame_length);
    }
    if ((bridge_mask & APP_BRIDGE_MASK_UART3) != 0U)
    {
        (void)App_RuntimeSendBytes(&huart3, frame, (uint16_t)frame_length);
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

static void App_AtHandleCommand(const AppAtCommand *command)
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
    case APP_AT_COMMAND_SET_POWER:
        output_result = App_OutputSetPower(command->data.power.rail,
                                           command->data.power.enabled);
        break;
    case APP_AT_COMMAND_GET_STATUS:
    {
        char status[APP_AT_PROTOCOL_MAX_LINE_LENGTH];
        size_t status_length = 0U;
        if (App_StatusEncode(App_OutputGetState(), status, sizeof(status), &status_length))
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
        break;
    case APP_AT_COMMAND_SET_BRIDGE:
        App_RuntimeSetBridgeEnabled(command->data.bridge.target,
                                    command->data.bridge.enabled);
        success = true;
        break;
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
    if (output_result == APP_OUTPUT_OK)
    {
        success = true;
    }

    (void)App_RuntimeSendText(
        &huart1,
        success ? "OK\r\n" : "+ERROR:PARSE\r\n");
}

static void App_AtProcessFrame(const uint8_t *frame, size_t frame_length)
{
    AppAtCommand command = {0};
    const AppAtParseStatus parse_status =
        AppAtProtocol_ParseFrame(frame, frame_length, &command);

    if (parse_status == APP_AT_PARSE_NOT_AT)
    {
        App_AtForwardFrame(frame, frame_length);
        return;
    }

    if (parse_status != APP_AT_PARSE_OK)
    {
        App_AtSendParseError(parse_status);
        return;
    }

    App_AtHandleCommand(&command);
}

void App_SystemTask(void *argument)
{
    (void)argument;

    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

    for (;;)
    {
        osDelay(APP_SYSTEM_HEARTBEAT_DELAY_MS);
    }
}

void App_AtTask(void *argument)
{
    char line_buffer[APP_AT_PROTOCOL_MAX_LINE_LENGTH] = {0};
    AppLineReader line_reader = {0};
    uint8_t byte = 0U;

    (void)argument;
    if (!AppLineReader_Init(&line_reader, line_buffer, sizeof(line_buffer)))
    {
        Error_Handler();
    }
    App_RuntimeStartUart1Receive();

    for (;;)
    {
        App_RuntimeWaitForUart1Data();

        if (App_RuntimeConsumeRxOverflow(1U))
        {
            AppLineReader_Reset(&line_reader);
            (void)App_RuntimeSendText(&huart1, "+ERROR:RX_OVERFLOW\r\n");
            continue;
        }

        while (App_RuntimePopRxByte(1U, &byte))
        {
            const AppLineReaderStatus line_status =
                AppLineReader_Push(&line_reader, byte);

            if (line_status == APP_LINE_READER_TOO_LONG)
            {
                (void)App_RuntimeSendText(&huart1, "+ERROR:LINE_TOO_LONG\r\n");
                continue;
            }

            if (line_status == APP_LINE_READER_COMPLETE)
            {
                App_AtProcessFrame(
                    (const uint8_t *)AppLineReader_GetLine(&line_reader),
                    AppLineReader_GetLineLength(&line_reader));
                AppLineReader_Reset(&line_reader);
            }
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
