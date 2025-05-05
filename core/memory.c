#include "core/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void abort_no_mem(void)
{
    fprintf(stderr, "Aborting: out of memory\n");
    exit(EXIT_FAILURE);
}

void* malloc_or_abort(size_t size)
{
    void* p = malloc(size);
    if (!p) abort_no_mem();
    return p;
}

void* realloc_or_abort(void* old_pointer, size_t size)
{
    void* p = realloc(old_pointer, size);
    if (!p) abort_no_mem();
    return p;
}

char* strdup_or_abort(const char* str)
{
    char* new_str = strdup(str);
    if (!new_str) abort_no_mem();
    return new_str;
}

