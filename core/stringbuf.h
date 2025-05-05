#pragma once

#include <stddef.h>
#include <stdbool.h>

struct tb_stringbuf
{
    char* data;
    size_t size;
    size_t capacity;
};

typedef struct tb_stringbuf tb_stringbuf;

tb_stringbuf tb_stringbuf_new(size_t capacity);
bool tb_stringbuf_cat(tb_stringbuf* buffer, const char* src, size_t len);
bool tb_stringbuf_cat_char(tb_stringbuf* buffer, char character);
void tb_stringbuf_clear(tb_stringbuf* buffer);
bool tb_stringbuf_empty(const tb_stringbuf* buffer);
void tb_stringbuf_destroy(tb_stringbuf* buffer);
