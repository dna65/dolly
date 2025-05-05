#include "disassembler/disassemble.h"

#include "core/core.h"

#include <stdlib.h>

const char* dolly_dsm_error_msg(dolly_dsm_status status)
{
    switch (status) {
    default: case DOLLY_DSM_OKAY: return "";
    case DOLLY_DSM_INVALID_OPCODE: return "invalid opcode encountered";
    }
}

dolly_dsm_list dolly_dsm_list_new(void)
{
    static const size_t DSM_START_CAPACITY = 16;
    dolly_dsm_list list = {
        .data = malloc_or_abort(sizeof(dolly_dsm_opcode) * DSM_START_CAPACITY),
        .size = 0,
        .capacity = DSM_START_CAPACITY,
        .last_offset = 0,
        .label_num = 0
    };

    return list;
}

dolly_dsm_opcode* dolly_dsm_list_append_empty(dolly_dsm_list* list)
{
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->data
            = realloc_or_abort(list->data,
                               sizeof(dolly_dsm_opcode) * list->capacity);
    }

    return list->data + ++list->size - 1;
}

static bool is_valid_opcode(dolly_opcode op)
{
    return !(op.instr == DOLLY_INVALID_INSTRUCTION
             || op.a_mode == DOLLY_INVALID_ADDR_MODE);
}

dolly_dsm_status dolly_dsm_list_read(dolly_dsm_list* list,
                                     const uint8_t* instructions, size_t len)
{
    // First pass
    size_t i = 0;
    while (i < len) {
        dolly_dsm_opcode* d_op = dolly_dsm_list_append_empty(list);
        d_op->op = dolly_resolve_opcode(instructions[i]);
        d_op->label = NULL;
        d_op->operand_label = NULL;
        d_op->offset = list->last_offset + i;
        if (!is_valid_opcode(d_op->op)) {
            i += 1;
            continue;
        }
        int operand_size = dolly_get_operand_size(d_op->op.a_mode);
        switch (operand_size) {
        case 1:
            d_op->operand = instructions[i + 1];
            break;
        case 2:
            d_op->operand = (instructions[i + 2] << 8) + instructions[i + 1];
            break;
        default: break;
        }
        i += 1 + operand_size;
    }
    list->last_offset += i;

    // Second pass: label generation
    for (size_t j = 0; j < list->size; ++j) {
        if (list->data[j].op.a_mode != RELATIVE) continue;

        uint16_t offs_to_find = list->data[j].offset + 2
                              + (int8_t)list->data[j].operand;
        for (size_t k = 0; k < list->size; ++k) {
            if (list->data[k].offset != offs_to_find) continue;

            if (list->data[k].label == NULL) {
                list->data[k].label = malloc_or_abort(24);
                sprintf(list->data[k].label, "LBL_%d", list->label_num++);
            }
            list->data[j].operand_label = list->data[k].label;
        }
    }

    return DOLLY_DSM_OKAY;
}

void dolly_dsm_list_write(const dolly_dsm_list* list, FILE* stream,
                          uint16_t offset)
{
    for (size_t i = 0; i < list->size; ++i) {
        fprintf(stream, "0x%04x\t%s%s\t", list->data[i].offset + offset,
                list->data[i].label ? list->data[i].label : "",
                list->data[i].label ? ":" : "");
        dolly_dsm_opcode_str(&list->data[i], stream);
        fprintf(stream, "\n");
    }
}

void dolly_dsm_list_destroy(dolly_dsm_list* list)
{
    for (size_t i = 0; i < list->size; ++i) {
        if (list->data[i].label) free(list->data[i].label);
    }
    free(list->data);
}

void dolly_dsm_opcode_str(const dolly_dsm_opcode* dolly_dsm_op, FILE* stream)
{
    fprintf(stream, "%s ", dolly_get_instr_name(dolly_dsm_op->op.instr));
    switch (dolly_dsm_op->op.a_mode) {
    case RELATIVE:
        fprintf(stream, "%s", dolly_dsm_op->operand_label);
        break;
    case IMMEDIATE:
        fprintf(stream, "#$%02x", dolly_dsm_op->operand);
        break;
    case IMPLICIT:
        break;
    case ACCUMULATOR:
        fprintf(stream, "A");
    case ZERO_PAGE:
        fprintf(stream, "$%02x", dolly_dsm_op->operand);
        break;
    case ZERO_PAGE_X:
        fprintf(stream, "$%02x,x", dolly_dsm_op->operand);
        break;
    case ZERO_PAGE_Y:
        fprintf(stream, "$%02x,y", dolly_dsm_op->operand);
        break;
    case ABSOLUTE:
        fprintf(stream, "$%04x", dolly_dsm_op->operand);
        break;
    case INDIRECT:
        fprintf(stream, "($%04x)", dolly_dsm_op->operand);
        break;
    case ABSOLUTE_X:
        fprintf(stream, "$%04x,x", dolly_dsm_op->operand);
        break;
    case ABSOLUTE_Y:
        fprintf(stream, "$%04x,y", dolly_dsm_op->operand);
        break;
    case INDIRECT_X:
        fprintf(stream, "($%02x,x)", dolly_dsm_op->operand);
        break;
    case INDIRECT_Y:
        fprintf(stream, "($%02x),y", dolly_dsm_op->operand);
        break;
    }
}
