#ifndef APP_UART_TUNNEL_H
#define APP_UART_TUNNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Encode UART2 or UART3 data as a CRLF-terminated uppercase hexadecimal event.
 */
bool App_UartTunnelEncodeEvent(uint8_t uart_index,
                               const uint8_t *payload,
                               size_t payload_length,
                               char *output,
                               size_t output_capacity,
                               size_t *output_length);

#endif
