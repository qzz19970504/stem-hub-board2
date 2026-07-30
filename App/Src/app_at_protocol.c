#include "app_at_protocol.h"

#include <string.h>

static bool AppAtProtocol_IsAtCandidate(const char *line)
{
    if (line == NULL)
    {
        return false;
    }

    return ((line[0] == 'A') || (line[0] == 'a'))
        && ((line[1] == 'T') || (line[1] == 't'));
}

static bool AppAtProtocol_HasValidFrame(const char *line, size_t *body_length)
{
    size_t line_length;
    size_t character_index;

    if ((line == NULL) || (body_length == NULL))
    {
        return false;
    }

    line_length = strlen(line);
    if ((line_length < 2U) || (line_length >= APP_AT_PROTOCOL_MAX_LINE_LENGTH))
    {
        return false;
    }

    if ((line[line_length - 2U] != '\r') || (line[line_length - 1U] != '\n'))
    {
        return false;
    }

    for (character_index = 0U; character_index < (line_length - 2U); ++character_index)
    {
        const char character = line[character_index];

        if ((character == ' ') || (character == '\t')
            || (character == '\r') || (character == '\n'))
        {
            return false;
        }

        if ((character >= 'a') && (character <= 'z'))
        {
            return false;
        }
    }

    *body_length = line_length - 2U;
    return true;
}

static bool AppAtProtocol_MatchAssignment(const char *prefix,
                                          const char *command_body,
                                          const char **value)
{
    size_t prefix_length;

    if ((prefix == NULL) || (command_body == NULL) || (value == NULL))
    {
        return false;
    }

    prefix_length = strlen(prefix);
    if (strncmp(command_body, prefix, prefix_length) != 0)
    {
        return false;
    }

    *value = command_body + prefix_length;
    return true;
}

static bool AppAtProtocol_ParseOnOff(const char *value, bool *enabled)
{
    if ((value == NULL) || (enabled == NULL))
    {
        return false;
    }

    if (strcmp(value, "ON") == 0)
    {
        *enabled = true;
        return true;
    }

    if (strcmp(value, "OFF") == 0)
    {
        *enabled = false;
        return true;
    }

    return false;
}

static AppAtParseStatus AppAtProtocol_ParsePwm(const char *value, uint8_t *percent)
{
    uint32_t parsed_percent = 0U;
    size_t digit_index;
    size_t value_length;

    if ((value == NULL) || (percent == NULL))
    {
        return APP_AT_PARSE_INVALID;
    }

    value_length = strlen(value);
    if ((value_length == 0U) || (value_length > 3U))
    {
        return APP_AT_PARSE_INVALID;
    }

    for (digit_index = 0U; digit_index < value_length; ++digit_index)
    {
        if ((value[digit_index] < '0') || (value[digit_index] > '9'))
        {
            return APP_AT_PARSE_INVALID;
        }

        parsed_percent = (parsed_percent * 10U) + (uint32_t)(value[digit_index] - '0');
    }

    if (parsed_percent > APP_PWM_MAX_PERCENT)
    {
        return APP_AT_PARSE_RANGE;
    }

    *percent = (uint8_t)parsed_percent;
    return APP_AT_PARSE_OK;
}

static bool AppAtProtocol_DecodeHexNibble(char character, uint8_t *nibble)
{
    if ((nibble == NULL) || (character < '0'))
    {
        return false;
    }

    if (character <= '9')
    {
        *nibble = (uint8_t)(character - '0');
        return true;
    }

    if ((character >= 'A') && (character <= 'F'))
    {
        *nibble = (uint8_t)(character - 'A' + 10);
        return true;
    }

    return false;
}

static AppAtParseStatus AppAtProtocol_ParseHex(const char *value,
                                               AppAtUartPayloadCommand *payload)
{
    size_t hex_length;
    size_t byte_index;

    if ((value == NULL) || (payload == NULL))
    {
        return APP_AT_PARSE_INVALID;
    }

    hex_length = strlen(value);
    if ((hex_length == 0U) || ((hex_length % 2U) != 0U))
    {
        return APP_AT_PARSE_INVALID;
    }

    if (hex_length > (APP_UART_TUNNEL_MAX_PAYLOAD_SIZE * 2U))
    {
        return APP_AT_PARSE_RANGE;
    }

    payload->length = hex_length / 2U;
    for (byte_index = 0U; byte_index < payload->length; ++byte_index)
    {
        uint8_t high_nibble;
        uint8_t low_nibble;

        if (!AppAtProtocol_DecodeHexNibble(value[byte_index * 2U], &high_nibble)
            || !AppAtProtocol_DecodeHexNibble(value[(byte_index * 2U) + 1U], &low_nibble))
        {
            payload->length = 0U;
            return APP_AT_PARSE_INVALID;
        }

        payload->bytes[byte_index] = (uint8_t)((high_nibble << 4U) | low_nibble);
    }

    return APP_AT_PARSE_OK;
}

static bool AppAtProtocol_MatchNmos(const char *command_body,
                                    AppAtCommand *out_command)
{
    static const char *const prefixes[] = {
        "AT+NMOS1=",
        "AT+NMOS2=",
        "AT+NMOS3=",
    };
    size_t prefix_index;

    for (prefix_index = 0U;
         prefix_index < (sizeof(prefixes) / sizeof(prefixes[0]));
         ++prefix_index)
    {
        const char *value = NULL;
        bool enabled = false;

        if (AppAtProtocol_MatchAssignment(prefixes[prefix_index], command_body, &value)
            && AppAtProtocol_ParseOnOff(value, &enabled))
        {
            out_command->type = APP_AT_COMMAND_SET_NMOS;
            out_command->data.nmos.index = (uint8_t)(prefix_index + 1U);
            out_command->data.nmos.enabled = enabled;
            return true;
        }
    }

    return false;
}

static bool AppAtProtocol_MatchBridge(const char *command_body,
                                      AppAtCommand *out_command)
{
    static const char *const prefixes[] = {
        "AT+UART2=",
        "AT+UART3=",
        "AT+UART2&3=",
    };
    static const AppBridgeTarget targets[] = {
        APP_BRIDGE_TARGET_UART2,
        APP_BRIDGE_TARGET_UART3,
        APP_BRIDGE_TARGET_UART23,
    };
    size_t prefix_index;

    for (prefix_index = 0U;
         prefix_index < (sizeof(prefixes) / sizeof(prefixes[0]));
         ++prefix_index)
    {
        const char *value = NULL;
        bool enabled = false;

        if (AppAtProtocol_MatchAssignment(prefixes[prefix_index], command_body, &value)
            && AppAtProtocol_ParseOnOff(value, &enabled))
        {
            out_command->type = APP_AT_COMMAND_SET_BRIDGE;
            out_command->data.bridge.target = targets[prefix_index];
            out_command->data.bridge.enabled = enabled;
            return true;
        }
    }

    return false;
}

AppAtParseStatus AppAtProtocol_ParseLine(const char *line, AppAtCommand *out_command)
{
    char command_body[APP_AT_PROTOCOL_MAX_LINE_LENGTH];
    const char *value = NULL;
    size_t body_length = 0U;
    AppAtParseStatus parse_status;

    if ((line == NULL) || (out_command == NULL))
    {
        return APP_AT_PARSE_INVALID;
    }

    if (!AppAtProtocol_HasValidFrame(line, &body_length))
    {
        return AppAtProtocol_IsAtCandidate(line)
            ? APP_AT_PARSE_INVALID
            : APP_AT_PARSE_NOT_AT;
    }

    if ((body_length < 3U) || (strncmp(line, "AT+", 3U) != 0))
    {
        return AppAtProtocol_IsAtCandidate(line)
            ? APP_AT_PARSE_INVALID
            : APP_AT_PARSE_NOT_AT;
    }

    (void)memset(out_command, 0, sizeof(*out_command));
    (void)memcpy(command_body, line, body_length);
    command_body[body_length] = '\0';

    if (AppAtProtocol_MatchNmos(command_body, out_command)
        || AppAtProtocol_MatchBridge(command_body, out_command))
    {
        return APP_AT_PARSE_OK;
    }

    if (AppAtProtocol_MatchAssignment("AT+PWM=", command_body, &value))
    {
        parse_status = AppAtProtocol_ParsePwm(value, &out_command->data.pwm.percent);
        if (parse_status == APP_AT_PARSE_OK)
        {
            out_command->type = APP_AT_COMMAND_SET_PWM;
        }
        return parse_status;
    }

    if (AppAtProtocol_MatchAssignment("AT+UARTTX=", command_body, &value))
    {
        parse_status = AppAtProtocol_ParseHex(value, &out_command->data.uart_payload);
        if (parse_status == APP_AT_PARSE_OK)
        {
            out_command->type = APP_AT_COMMAND_SEND_UART;
        }
        return parse_status;
    }

    return APP_AT_PARSE_INVALID;
}
