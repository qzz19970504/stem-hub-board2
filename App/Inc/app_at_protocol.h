#ifndef APP_AT_PROTOCOL_H
#define APP_AT_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_config.h"
#include "app_power.h"

typedef enum
{
    APP_AT_PARSE_OK = 0,
    APP_AT_PARSE_NOT_AT,
    APP_AT_PARSE_INVALID,
    APP_AT_PARSE_RANGE
} AppAtParseStatus;

typedef enum
{
    APP_AT_COMMAND_NONE = 0,
    APP_AT_COMMAND_SET_NMOS,
    APP_AT_COMMAND_SET_PWM,
    APP_AT_COMMAND_SET_PWM_TIME,
    APP_AT_COMMAND_SET_BREATH_TEST,
    APP_AT_COMMAND_SET_POWER,
    APP_AT_COMMAND_GET_STATUS,
    APP_AT_COMMAND_START_TRANSPARENT,
    APP_AT_COMMAND_SEND_UART
} AppAtCommandType;

typedef enum
{
    APP_BRIDGE_TARGET_UART2 = 0,
    APP_BRIDGE_TARGET_UART3,
    APP_BRIDGE_TARGET_UART23
} AppBridgeTarget;

typedef struct
{
    uint8_t index;
    bool enabled;
} AppAtNmosCommand;

typedef struct
{
    uint8_t percent;
} AppAtPwmCommand;

typedef struct { uint16_t milliseconds; } AppAtPwmTimeCommand;
typedef struct { bool enabled; } AppAtBreathCommand;

typedef struct
{
    AppPowerRail rail;
    bool enabled;
} AppAtPowerCommand;

typedef struct
{
    AppBridgeTarget target;
} AppAtTransparentCommand;

typedef struct
{
    uint8_t bytes[APP_UART_TUNNEL_MAX_PAYLOAD_SIZE];
    size_t length;
} AppAtUartPayloadCommand;

typedef struct
{
    AppAtCommandType type;
    union
    {
        AppAtNmosCommand nmos;
        AppAtPwmCommand pwm;
        AppAtPwmTimeCommand pwm_time;
        AppAtBreathCommand breath;
        AppAtPowerCommand power;
        AppAtTransparentCommand transparent;
        AppAtUartPayloadCommand uart_payload;
    } data;
} AppAtCommand;

/**
 * Parse one CRLF-terminated UART1 line.
 *
 * Returns a classification or validation result and fills out_command only
 * when APP_AT_PARSE_OK is returned. This function performs no I/O.
 */
AppAtParseStatus AppAtProtocol_ParseLine(const char *line, AppAtCommand *out_command);

/**
 * Parse one CRLF-terminated frame with an explicit byte length.
 *
 * Embedded NUL bytes are never treated as string terminators. A binary frame
 * that is not an AT candidate is classified as APP_AT_PARSE_NOT_AT.
 */
AppAtParseStatus AppAtProtocol_ParseFrame(const uint8_t *frame,
                                          size_t frame_length,
                                          AppAtCommand *out_command);

#endif
