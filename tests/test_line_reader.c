#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "app_line_reader.h"

static AppLineReaderStatus push_text(AppLineReader *reader, const char *text)
{
    AppLineReaderStatus status = APP_LINE_READER_NONE;
    size_t character_index;

    for (character_index = 0U; text[character_index] != '\0'; ++character_index)
    {
        status = AppLineReader_Push(reader, (uint8_t)text[character_index]);
    }

    return status;
}

int main(void)
{
    char line[8] = {0};
    AppLineReader reader = {0};

    assert(AppLineReader_Init(&reader, line, sizeof(line)));
    assert(push_text(&reader, "AT\r\n") == APP_LINE_READER_COMPLETE);
    assert(strcmp(AppLineReader_GetLine(&reader), "AT\r\n") == 0);

    AppLineReader_Reset(&reader);
    assert(push_text(&reader, "123456\r\n") == APP_LINE_READER_TOO_LONG);
    assert(push_text(&reader, "B\r\n") == APP_LINE_READER_COMPLETE);
    assert(strcmp(AppLineReader_GetLine(&reader), "B\r\n") == 0);

    AppLineReader_Reset(&reader);
    assert(push_text(&reader, "1234567X") == APP_LINE_READER_TOO_LONG);
    assert(push_text(&reader, "junk\r\n") == APP_LINE_READER_NONE);
    assert(push_text(&reader, "C\r\n") == APP_LINE_READER_COMPLETE);

    assert(!AppLineReader_Init(NULL, line, sizeof(line)));
    assert(!AppLineReader_Init(&reader, NULL, sizeof(line)));
    assert(!AppLineReader_Init(&reader, line, 1U));

    return 0;
}
