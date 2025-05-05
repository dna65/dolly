#include "assembler/parse.h"

#include <stdlib.h>
#include <string.h>

bool only_absolute(dolly_instruction instr);
dolly_addressing_mode get_amode(const dolly_asm_instruction* instr,
                                uint16_t value, bool is_label);

dolly_addressing_mode get_amode(const dolly_asm_instruction* instr,
                                uint16_t value, bool is_label)
{

    switch (instr->operand_type) {
    case DOLLY_ASM_OPERAND_INTEGER:
        if (only_absolute(instr->instr)) return ABSOLUTE;
        return value > UINT8_MAX ? ABSOLUTE : ZERO_PAGE;
    case DOLLY_ASM_OPERAND_IDENTIFIER:
        if (dolly_is_branch(instr->instr)) return RELATIVE;
        if (only_absolute(instr->instr)) return ABSOLUTE;
        return (is_label || value > UINT8_MAX) ? ABSOLUTE : ZERO_PAGE;
    case DOLLY_ASM_OPERAND_INTEGER_X:
        return value > UINT8_MAX ? ABSOLUTE_X : ZERO_PAGE_X;
    case DOLLY_ASM_OPERAND_INTEGER_Y:
        return value > UINT8_MAX ? ABSOLUTE_Y : ZERO_PAGE_Y;
    case DOLLY_ASM_OPERAND_IDENTIFIER_X:
        return (is_label || value > UINT8_MAX) ? ABSOLUTE_X : ZERO_PAGE_X;
    case DOLLY_ASM_OPERAND_IDENTIFIER_Y:
        return (is_label || value > UINT8_MAX) ? ABSOLUTE_Y : ZERO_PAGE_Y;
    case DOLLY_ASM_OPERAND_RELATIVE_INT:
    case DOLLY_ASM_OPERAND_RELATIVE_IDEN:
        return RELATIVE;
    case DOLLY_ASM_OPERAND_INDIRECT_INDEXED_INT:
        return value > UINT8_MAX ? DOLLY_INVALID_ADDR_MODE : INDIRECT_Y;
    case DOLLY_ASM_OPERAND_INDEXED_INDIRECT_INT:
        return value > UINT8_MAX ? DOLLY_INVALID_ADDR_MODE : INDIRECT_X;
    case DOLLY_ASM_OPERAND_INDIRECT_INDEXED_IDEN:
        return (is_label || value > UINT8_MAX) ? DOLLY_INVALID_ADDR_MODE : INDIRECT_Y;
    case DOLLY_ASM_OPERAND_INDEXED_INDIRECT_IDEN:
        return (is_label || value > UINT8_MAX) ? DOLLY_INVALID_ADDR_MODE : INDIRECT_Y;
    case DOLLY_ASM_OPERAND_ACCUMULATOR:
        return ACCUMULATOR;
    case DOLLY_ASM_OPERAND_IMMEDIATE_INT:
        return value > UINT8_MAX ? DOLLY_INVALID_ADDR_MODE : IMMEDIATE;
    case DOLLY_ASM_OPERAND_IMMEDIATE_IDEN:
        return (is_label || value > UINT8_MAX) ? DOLLY_INVALID_ADDR_MODE : IMMEDIATE;
    case DOLLY_ASM_OPERAND_INDIRECT_INT:
    case DOLLY_ASM_OPERAND_INDIRECT_IDEN:
        return INDIRECT;
    case DOLLY_ASM_OPERAND_IMPLICIT:
        return IMPLICIT;
    default: return DOLLY_INVALID_ADDR_MODE;
    }
}

bool only_absolute(dolly_instruction instr)
{
    return instr == JMP || instr == JSR;
}

const dolly_addressing_mode AMODE_GROUP_1
    = IMMEDIATE | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X | ABSOLUTE_Y
    | INDIRECT_X | INDIRECT_Y;

const dolly_addressing_mode COMPATIBLE_ADDR_MODES[DOLLY_6502_INSTRUCTION_COUNT] = {
    [ADC] = AMODE_GROUP_1,
    [AND] = AMODE_GROUP_1,
    [ASL] = ACCUMULATOR | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [BCC] = RELATIVE,
    [BCS] = RELATIVE,
    [BEQ] = RELATIVE,
    [BIT] = ZERO_PAGE | ABSOLUTE,
    [BMI] = RELATIVE,
    [BNE] = RELATIVE,
    [BPL] = RELATIVE,
    [BRK] = IMMEDIATE,
    [BVC] = RELATIVE,
    [BVS] = RELATIVE,
    [CLC] = IMMEDIATE,
    [CLI] = IMMEDIATE,
    [CLV] = IMMEDIATE,
    [CMP] = AMODE_GROUP_1,
    [CPX] = IMMEDIATE | ZERO_PAGE | ABSOLUTE,
    [CPY] = IMMEDIATE | ZERO_PAGE | ABSOLUTE,
    [DEC] = ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [DEX] = IMMEDIATE,
    [DEY] = IMMEDIATE,
    [EOR] = AMODE_GROUP_1,
    [INC] = ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [INX] = IMMEDIATE,
    [INY] = IMMEDIATE,
    [JMP] = ABSOLUTE | INDIRECT,
    [JSR] = ABSOLUTE,
    [LDX] = IMMEDIATE | ZERO_PAGE | ZERO_PAGE_Y | ABSOLUTE | ABSOLUTE_Y,
    [LDY] = IMMEDIATE | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [LSR] = ACCUMULATOR | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [NOP] = IMMEDIATE,
    [ORA] = AMODE_GROUP_1,
    [PHA] = IMMEDIATE,
    [PHP] = IMMEDIATE,
    [PLA] = IMMEDIATE,
    [PLP] = IMMEDIATE,
    [ROL] = ACCUMULATOR | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [ROR] = ACCUMULATOR | ZERO_PAGE | ZERO_PAGE_X | ABSOLUTE | ABSOLUTE_X,
    [RTI] = IMMEDIATE,
    [RTS] = IMMEDIATE,
    [SBC] = AMODE_GROUP_1,
    [SED] = IMMEDIATE,
    [SEI] = IMMEDIATE,
    [STA] = AMODE_GROUP_1 ^ IMMEDIATE,
    [STX] = ABSOLUTE | ZERO_PAGE | ZERO_PAGE_Y,
    [STY] = ABSOLUTE | ZERO_PAGE | ZERO_PAGE_X,
    [TAX] = IMMEDIATE,
    [TAY] = IMMEDIATE,
    [TSX] = IMMEDIATE,
    [TXA] = IMMEDIATE,
    [TXS] = IMMEDIATE,
    [TYA] = IMMEDIATE,
    [SEC] = IMMEDIATE,
    [LDA] = AMODE_GROUP_1,
    [CLD] = IMMEDIATE
};

bool dolly_asm_verify_semantics(dolly_asm_context* ctx,
                                dolly_asm_syntax_tree* input)
{
    uint32_t bin_offset = 0;
    uint8_t section_number = 0;
    // First pass
    for (size_t index = 0; index < input->size; ++index) {
        dolly_asm_syntax_node* node = &input->nodes[index];
        node->bin_offset = bin_offset;
        node->section_number = section_number;
        switch (node->type) {
        case DOLLY_ASM_NODE_ORIGIN: {
            const dolly_asm_syntax_node* last
                = dolly_asm_syntax_tree_rfind(input, node,
                    DOLLY_ASM_NODE_WRITABLE);
            if (last != NULL && last->bin_offset >= node->origin_offset
                && node->section_number == last->section_number) {
                dolly_asm_report_error_node(ctx, node);
                printf("Origin directives cannot go backwards within "
                       "a section\n");
                break;
            }

            bin_offset = node->origin_offset;
            break;
        }
        case DOLLY_ASM_NODE_BYTE_DATA:
            bin_offset += node->directive.size;
            break;
        case DOLLY_ASM_NODE_STRING: {
            uint32_t string_bytes_len = strlen(node->identifier.name) + 1;
            bin_offset += string_bytes_len;
            break;
        }
        case DOLLY_ASM_NODE_INSTRUCTION: {
            if (node->instruction.operand_type == DOLLY_ASM_OPERAND_IMPLICIT) {
                node->instruction.a_mode = IMPLICIT;
                bin_offset += 1;
                break;
            }
            const char* iden_name = node->instruction.operand.identifier;
            uint16_t value = node->instruction.operand.integer;
            bool is_label = false;
            if (node->instruction.operand.is_identifier) {
                // Check existence
                const tb_hash_node* entry
                    = tb_hash_table_get(&input->identifiers, iden_name);
                if (entry == NULL) {
                    dolly_asm_report_error_node(ctx, node);
                    printf("No identifier named '%s' defined\n", iden_name);
                    break;
                }
                const dolly_asm_syntax_node* iden_node
                    = &input->nodes[entry->int_value];
                is_label = iden_node->type == DOLLY_ASM_NODE_LABEL;
                if (!is_label) {
                    value = iden_node->identifier.addr_or_const;
                }
            }
            dolly_addressing_mode amode
                = get_amode(&node->instruction, value, is_label);

            if (!(COMPATIBLE_ADDR_MODES[node->instruction.instr] & amode)) {
                dolly_asm_report_error_node(ctx, node);
                printf("Incompatible addressing mode: Instruction '%s' "
                       "does not support %s mode\n",
                       dolly_get_instr_name(node->instruction.instr),
                       dolly_get_amode_name(amode));
            }
            node->instruction.a_mode = amode;
            uint32_t advance_by = 1 + dolly_get_operand_size(amode);
            bin_offset += advance_by;
            break;
        }
        case DOLLY_ASM_NODE_CONSTANT:
        case DOLLY_ASM_NODE_LABEL:
        case DOLLY_ASM_NODE_SENTINEL:
            break;
        case DOLLY_ASM_NODE_SECTION_TEXT:
        case DOLLY_ASM_NODE_SECTION_DATA:
            ++section_number;
            break;
        default:
            break;
        }
    }

    if (ctx->errors > 0) return false;

    // Second pass - branch checking

    for (size_t index = 0; index < input->size; ++index) {
        dolly_asm_syntax_node* node = &input->nodes[index];
        if (node->type != DOLLY_ASM_NODE_INSTRUCTION) continue;
        if (!dolly_is_branch(node->instruction.instr)) continue;
        if (!node->instruction.operand.is_identifier) continue;

        const char* iden_name = node->instruction.operand.identifier;
        const tb_hash_node* entry
                    = tb_hash_table_get(&input->identifiers, iden_name);
        const dolly_asm_syntax_node* iden_node
                    = &input->nodes[entry->int_value];
        if (iden_node->type != DOLLY_ASM_NODE_LABEL) continue;
        int branch_distance
            = (int)iden_node->bin_offset - (int)node->bin_offset;
        if (branch_distance < -126 || branch_distance > 129) {
            dolly_asm_report_error_node(ctx, node);
            printf("Label '%s' is out of range of branch\n",
                    iden_name);
            printf("  (Branch distance: %d bytes)\n", abs(branch_distance));
        }
    }

    return ctx->errors == 0;
}
