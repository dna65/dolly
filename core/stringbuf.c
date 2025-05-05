#include "stringbuf.h"

#include <stdlib.h>
#include <string.h>

static size_t min(size_t a, size_t b);

static size_t min(size_t a, size_t b)
{
    return a < b ? a : b;
}

tb_stringbuf tb_stringbuf_new(size_t capacity)
{
    tb_stringbuf buffer = {
        .data = capacity ? malloc(capacity) : NULL,
        .size = 0,
        .capacity = capacity
    };

    return buffer;
}

bool tb_stringbuf_cat(tb_stringbuf* buffer, const char* src, size_t len)
{
    size_t len_to_copy = min(strlen(src), len);

    if (buffer->size + len_to_copy + 1 >= buffer->capacity) {
        buffer->capacity += len_to_copy + 1;
        void* ptr = realloc(buffer->data, buffer->capacity);
        if (!ptr) return false;
        buffer->data = ptr;
    }

    memcpy(buffer->data + buffer->size, src, len_to_copy);
    buffer->size += len_to_copy;
    buffer->data[buffer->size] = '\0';

    return true;
}

bool tb_stringbuf_cat_char(tb_stringbuf* buffer, char character)
{
    if (buffer->size + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        void* ptr = realloc(buffer->data, buffer->capacity);
        if (!ptr) return false;
        buffer->data = ptr;
    }

    buffer->data[buffer->size++] = character;
    buffer->data[buffer->size] = '\0';

    return true;
}

void tb_stringbuf_clear(tb_stringbuf* buffer)
{
    buffer->size = 0;
    if (buffer->capacity > 0) buffer->data[0] = '\0';
}

bool tb_stringbuf_empty(const tb_stringbuf* buffer)
{
    return buffer->size == 0;
}

void tb_stringbuf_destroy(tb_stringbuf* buffer)
{
    if (buffer->data) free(buffer->data);
}
