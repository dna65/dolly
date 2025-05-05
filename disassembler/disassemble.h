#pragma once

#include <stddef.h>
#include <stdio.h>

#include "virtual-machine/cpu.h"
#include "core/core.h"

struct dolly_dsm_opcode
{
    char* label;
    dolly_opcode op;
    uint16_t offset;
    uint16_t operand;
    const char* operand_label;
};

typedef struct dolly_dsm_opcode dolly_dsm_opcode;

struct dolly_dsm_list
{
    dolly_dsm_opcode* data;
    size_t size;
    size_t capacity;
    uint16_t last_offset;
    int label_num;
};

typedef struct dolly_dsm_list dolly_dsm_list;

enum dolly_dsm_status
{
    DOLLY_DSM_OKAY, DOLLY_DSM_INVALID_OPCODE
};

typedef enum dolly_dsm_status dolly_dsm_status;

const char* dolly_dsm_error_msg(dolly_dsm_status status);

dolly_dsm_list    dolly_dsm_list_new(void);
dolly_dsm_opcode* dolly_dsm_list_append_empty(dolly_dsm_list* list);
dolly_dsm_status  dolly_dsm_list_read(dolly_dsm_list* list,
                                      const uint8_t* instructions,
                                      size_t len);

void dolly_dsm_list_write(const dolly_dsm_list* list, FILE* stream,
                          uint16_t offset);
void dolly_dsm_list_destroy(dolly_dsm_list* list);

void dolly_dsm_opcode_str(const dolly_dsm_opcode* dsm_op, FILE* stream);
