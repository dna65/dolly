#include "core/streambuf.h"

#include <stdlib.h>

tb_streambuf tb_streambuf_new(size_t capacity)
{
    tb_streambuf buffer = {
        .data = capacity ? malloc(capacity) : NULL,
        .size = 0,
        .capacity = capacity
    };

    return buffer;
}

tb_streambuf_status tb_streambuf_init(tb_streambuf* buffer, size_t capacity)
{
    buffer->data = capacity ? malloc(capacity) : NULL;
    buffer->size = 0;
    buffer->capacity = capacity;

    if (tb_streambuf_is_ok(buffer) == false) return TB_STREAMBUF_NOMEM;
    return TB_STREAMBUF_OKAY;
}

bool tb_streambuf_is_ok(const tb_streambuf* buffer)
{
    return !(buffer->data == NULL && buffer->capacity != 0);
}

void tb_streambuf_clear(tb_streambuf* buffer)
{
    buffer->size = 0;
}

tb_streambuf_status tb_streambuf_read_all(tb_streambuf* buffer, FILE* file)
{
    while (feof(file) == false) {
        if (buffer->size >= buffer->capacity) {
            buffer->capacity *= 2;
            char* new_ptr = realloc(buffer->data, buffer->capacity);
            if (new_ptr == NULL) return TB_STREAMBUF_NOMEM;
            buffer->data = new_ptr;
        }

        size_t bytes_expected = buffer->capacity - buffer->size;
        size_t bytes_read = fread(buffer->data + buffer->size, 1,
                                  bytes_expected, file);
        buffer->size += bytes_read;
        if (bytes_read < bytes_expected) {
            return feof(file) ? TB_STREAMBUF_OKAY : TB_STREAMBUF_OTHER_ERROR;
        }
    }

    return TB_STREAMBUF_OKAY;
}

tb_streambuf_status tb_streambuf_read_some(tb_streambuf* buffer, FILE* file,
    size_t bytes)
{
    if (buffer->size + bytes > buffer->capacity) {
        buffer->capacity = buffer->size + bytes;
        char* new_ptr = realloc(buffer->data, buffer->capacity);
        if (new_ptr == NULL) return TB_STREAMBUF_NOMEM;
        buffer->data = new_ptr;
    }

    size_t bytes_read = fread(buffer->data + buffer->size, 1, bytes, file);
    buffer->size += bytes_read;
    if (bytes_read < bytes) {
        return feof(file) ? TB_STREAMBUF_EOF : TB_STREAMBUF_OTHER_ERROR;
    }

    return TB_STREAMBUF_OKAY;
}

tb_streambuf_status tb_streambuf_write(const tb_streambuf* buffer, FILE* file)
{
    size_t bytes_written = fwrite(buffer->data, 1, buffer->size, file);
    if (bytes_written < buffer->size) {
        return TB_STREAMBUF_OTHER_ERROR;
    }

    return TB_STREAMBUF_OKAY;
}

void tb_streambuf_destroy(tb_streambuf* buffer)
{
    if (buffer->data) free(buffer->data);
}

