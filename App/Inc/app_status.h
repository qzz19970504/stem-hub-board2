#ifndef APP_STATUS_H
#define APP_STATUS_H

#include <stdbool.h>
#include <stddef.h>
#include "app_output_state.h"

bool App_StatusEncode(const AppOutputState *state,
                      char *output,
                      size_t output_capacity,
                      size_t *output_length);

#endif
