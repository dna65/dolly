#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

struct tb_streambuf
{
    char* data;
    size_t size;
    size_t capacity;
};

typedef struct tb_streambuf tb_streambuf;

enum tb_streambuf_status
{
    TB_STREAMBUF_OKAY, TB_STREAMBUF_EOF, TB_STREAMBUF_NOMEM,
    TB_STREAMBUF_OTHER_ERROR
};

typedef enum tb_streambuf_status tb_streambuf_status;

tb_streambuf        tb_streambuf_new(size_t capacity);
tb_streambuf_status tb_streambuf_init(tb_streambuf* buffer, size_t capacity);
bool                tb_streambuf_is_ok(const tb_streambuf* buffer);
void                tb_streambuf_clear(tb_streambuf* buffer);
tb_streambuf_status tb_streambuf_read_all(tb_streambuf* buffer, FILE* file);
tb_streambuf_status tb_streambuf_read_some(tb_streambuf* buffer, FILE* file,
    size_t bytes);
tb_streambuf_status tb_streambuf_write(const tb_streambuf* buffer, FILE* file);
void                tb_streambuf_destroy(tb_streambuf* buffer);
