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

static void expect_transparent(const char *line,
                               AppBridgeTarget expected_target)
{
    AppAtCommand command = parse_ok(line);

    assert(command.type == APP_AT_COMMAND_START_TRANSPARENT);
    assert(command.data.transparent.target == expected_target);
}

static void expect_power(const char *line, AppPowerRail rail, bool enabled)
{
    AppAtCommand command = parse_ok(line);
    assert(command.type == APP_AT_COMMAND_SET_POWER);
    assert(command.data.power.rail == rail);
    assert(command.data.power.enabled == enabled);
}

int main(void)
{
    AppAtCommand command = {0};
    static const uint8_t binary_payload[] = {0x00U, 0xFFU, 0x10U};
    static const uint8_t binary_non_at_frame[] = {'P', 0x00U, 'Q', '\r', '\n'};
    static const uint8_t binary_at_frame[] = {
        'A', 'T', '+', 'P', 'W', 'M', '=', '1', 0x00U, '\r', '\n'
    };

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
    assert(AppAtProtocol_ParseLine("AT+PWM=1000\r\n", &command) == APP_AT_PARSE_RANGE);
    assert(AppAtProtocol_ParseLine("AT+PWM=-1\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+PWM=A\r\n", &command) == APP_AT_PARSE_INVALID);

    command = parse_ok("AT+PWM_TIME=0\r\n");
    assert(command.type == APP_AT_COMMAND_SET_PWM_TIME);
    assert(command.data.pwm_time.milliseconds == 0U);
    command = parse_ok("AT+PWM_TIME=10000\r\n");
    assert(command.data.pwm_time.milliseconds == 10000U);
    assert(AppAtProtocol_ParseLine("AT+PWM_TIME=10001\r\n", &command) == APP_AT_PARSE_RANGE);
    assert(AppAtProtocol_ParseLine("AT+PWM_TIME=-1\r\n", &command) == APP_AT_PARSE_INVALID);

    command = parse_ok("AT+BREATH_TEST=ON\r\n");
    assert(command.type == APP_AT_COMMAND_SET_BREATH_TEST);
    assert(command.data.breath.enabled);
    command = parse_ok("AT+BREATH_TEST=OFF\r\n");
    assert(!command.data.breath.enabled);
    assert(AppAtProtocol_ParseLine("AT+BREATH_TEST=on\r\n", &command) == APP_AT_PARSE_INVALID);

    expect_transparent("AT+TRANS=1\r\n", APP_BRIDGE_TARGET_UART2);
    expect_transparent("AT+TRANS=2\r\n", APP_BRIDGE_TARGET_UART3);
    expect_transparent("AT+TRANS=1&2\r\n", APP_BRIDGE_TARGET_UART23);
    assert(AppAtProtocol_ParseLine("AT+TRANS=\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+TRANS=3\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART2=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART2=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART3=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART3=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART2&3=ON\r\n", &command) == APP_AT_PARSE_INVALID);
    assert(AppAtProtocol_ParseLine("AT+UART2&3=OFF\r\n", &command) == APP_AT_PARSE_INVALID);
    expect_power("AT+12V=ON\r\n", APP_POWER_RAIL_12V, true);
    expect_power("AT+12V=OFF\r\n", APP_POWER_RAIL_12V, false);
    expect_power("AT+18V=ON\r\n", APP_POWER_RAIL_18V, true);
    expect_power("AT+18V=OFF\r\n", APP_POWER_RAIL_18V, false);
    command = parse_ok("AT+STATUS=?\r\n");
    assert(command.type == APP_AT_COMMAND_GET_STATUS);
    assert(AppAtProtocol_ParseLine("AT+STATUS?\r\n", &command) == APP_AT_PARSE_INVALID);

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
    assert(AppAtProtocol_ParseFrame(binary_non_at_frame,
                                    sizeof(binary_non_at_frame),
                                    &command)
           == APP_AT_PARSE_NOT_AT);
    assert(AppAtProtocol_ParseFrame(binary_at_frame,
                                    sizeof(binary_at_frame),
                                    &command)
           == APP_AT_PARSE_INVALID);

    return 0;
}
