#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_at_protocol.h"

static AppAtCommand parse_ok(const char *line)
{
    AppAtCommand command = {0};

    assert(AppAtProtocol_ParseLine(line, &command) == APP_AT_PARSE_OK);
    return command;
}

static void expect_nmos(const char *line, uint8_t expected_index, bool expected_enabled)
{
    AppAtCommand command = parse_ok(line);

    assert(command.type == APP_AT_COMMAND_SET_NMOS);
    assert(command.data.nmos.index == expected_index);
    assert(command.data.nmos.enabled == expected_enabled);
}

static void expect_bridge(const char *line,
                          AppBridgeTarget expected_target,
                          bool expected_enabled)
{
    AppAtCommand command = parse_ok(line);

    assert(command.type == APP_AT_COMMAND_SET_BRIDGE);
    assert(command.data.bridge.target == expected_target);
    assert(command.data.bridge.enabled == expected_enabled);
}

int main(void)
{
    AppAtCommand command = {0};
    static const uint8_t binary_payload[] = {0x00U, 0xFFU, 0x10U};

    expect_nmos("AT+NMOS1=ON\r\n", 1U, true);
    expect_nmos("AT+NMOS2=OFF\r\n", 2U, false);
    expect_nmos("AT+NMOS3=ON\r\n", 3U, true);

    command = parse_ok("AT+PWM=0\r\n");
    assert(command.type == APP_AT_COMMAND_SET_PWM);
    assert(command.data.pwm.percent == 0U);
    command = parse_ok("AT+PWM=1\r\n");
    assert(command.data.pwm.percent == 1U);
    command = parse_ok("AT+PWM=50\r\n");
    assert(command.data.pwm.percent == 50U);
    command = parse_ok("AT+PWM=100\r\n");
    assert(command.data.pwm.percent == 100U);
    assert(AppAtProtocol_ParseLine("AT+PWM=101\r\n", &command) == APP_AT_PARSE_RANGE);
    assert(AppAtProtocol_ParseLine("AT+PWM=-1\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+PWM=A\r\n", &command) == APP_AT_PARSE_INVALID);

    expect_bridge("AT+UART2=ON\r\n", APP_BRIDGE_TARGET_UART2, true);
    expect_bridge("AT+UART3=OFF\r\n", APP_BRIDGE_TARGET_UART3, false);
    expect_bridge("AT+UART2&3=ON\r\n", APP_BRIDGE_TARGET_UART23, true);

    command = parse_ok("AT+UARTTX=00FF10\r\n");
    assert(command.type == APP_AT_COMMAND_SEND_UART);
    assert(command.data.uart_payload.length == sizeof(binary_payload));
    assert(memcmp(command.data.uart_payload.bytes,
                  binary_payload,
                  sizeof(binary_payload)) == 0);
    command = parse_ok(
        "AT+UARTTX=000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F\r\n");
    assert(command.data.uart_payload.length == APP_UART_TUNNEL_MAX_PAYLOAD_SIZE);

    assert(AppAtProtocol_ParseLine("AT+UARTTX=\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UARTTX=0\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UARTTX=00ff\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UARTTX=00-G\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine(
               "AT+UARTTX=000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20\r\n",
               &command)
           == APP_AT_PARSE_RANGE);

    assert(AppAtProtocol_ParseLine("payload\r\n", &command) == APP_AT_PARSE_NOT_AT);
    assert(AppAtProtocol_ParseLine("AT+UNKNOWN=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+NMOS1=ON", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+NMOS1=ON\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT +NMOS1=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+NMOS1 =ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("at+NMOS1=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine(NULL, &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+PWM=1\r\n", NULL) == APP_AT_PARSE_INVALID);

    return 0;
}
