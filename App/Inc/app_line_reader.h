#ifndef APP_LINE_READER_H
#define APP_LINE_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    APP_LINE_READER_NONE = 0,
    APP_LINE_READER_COMPLETE,
    APP_LINE_READER_TOO_LONG
} AppLineReaderStatus;

typedef struct
{
    char *buffer;
    size_t capacity;
    size_t length;
    bool is_discarding;
    bool previous_byte_was_carriage_return;
    bool has_complete_line;
} AppLineReader;

/** Initialize a CRLF line accumulator over caller-owned storage. */
bool AppLineReader_Init(AppLineReader *reader, char *buffer, size_t capacity);

/** Consume one byte and report completed or overlong lines. */
AppLineReaderStatus AppLineReader_Push(AppLineReader *reader, uint8_t byte);

/** Return the completed NUL-terminated line, or NULL when incomplete. */
const char *AppLineReader_GetLine(const AppLineReader *reader);

/** Return the completed line length in bytes, or zero when incomplete. */
size_t AppLineReader_GetLineLength(const AppLineReader *reader);

/** Clear accumulated state and leave the reader ready for a new line. */
void AppLineReader_Reset(AppLineReader *reader);

#endif
