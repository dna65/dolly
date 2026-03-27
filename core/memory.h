#pragma once

#include <stddef.h>

#include "core/def.h"

void abort_no_mem(void) __COLD__;

void* malloc_or_abort(size_t size);
void* realloc_or_abort(void* old_pointer, size_t size);
char* strdup_or_abort(const char* str);
