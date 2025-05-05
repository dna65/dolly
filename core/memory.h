#pragma once

#include <stddef.h>

void abort_no_mem(void);

void* malloc_or_abort(size_t size);
void* realloc_or_abort(void* old_pointer, size_t size);
char* strdup_or_abort(const char* str);
