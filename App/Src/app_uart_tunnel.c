#include "app_uart_tunnel.h"

#include <string.h>

#include "app_config.h"

bool App_UartTunnelEncodeEvent(uint8_t uart_index,
                               const uint8_t *payload,
                               size_t payload_length,
                               char *output,
                               size_t output_capacity,
                               size_t *output_length)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    const char *prefix;
    size_t prefix_length;
    size_t required_length;
    size_t payload_index;

    if ((payload == NULL) || (payload_length == 0U)
        || (payload_length > APP_UART_TUNNEL_MAX_PAYLOAD_SIZE)
        || (output == NULL) || (output_length == NULL))
    {
        return false;
    }

    if (uart_index == 2U)
    {
        prefix = "+UART2RX:";
    }
    else if (uart_index == 3U)
    {
        prefix = "+UART3RX:";
    }
    else
    {
        return false;
    }

    prefix_length = strlen(prefix);
    required_length = prefix_length + (payload_length * 2U) + 2U;
    if (output_capacity <= required_length)
    {
        return false;
    }

    (void)memcpy(output, prefix, prefix_length);
    for (payload_index = 0U; payload_index < payload_length; ++payload_index)
    {
        const uint8_t byte = payload[payload_index];
        const size_t output_index = prefix_length + (payload_index * 2U);

        output[output_index] = hex_digits[byte >> 4U];
        output[output_index + 1U] = hex_digits[byte & 0x0FU];
    }

    output[required_length - 2U] = '\r';
    output[required_length - 1U] = '\n';
    output[required_length] = '\0';
    *output_length = required_length;
    return true;
}
