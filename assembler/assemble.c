#include "parse.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t dumb_opcode(dolly_opcode op);

// TODO: Better approach
static uint8_t dumb_opcode(dolly_opcode op)
{
    for (int i = 0; i < 256; ++i) {
        dolly_opcode o = dolly_resolve_opcode(i);
        if (o.a_mode == op.a_mode && o.instr == op.instr) return i;
    }
    assert(0); // Shouldn't be reached
    return 0xEA;
}

bool dolly_asm_make_executable(dolly_asm_syntax_tree* input,
                               dolly_executable* output)
{
    dolly_executable_init(output);
    output->header.arch = DOLLY_ARCH_6502;

    // First pass - map out sections
    for (size_t index = 0; index < input->size; ++index) {
        if (index == input->size - 1) continue;

        dolly_asm_syntax_node* sect_node = &input->nodes[index];
        if (sect_node->type != DOLLY_ASM_NODE_SECTION_TEXT
            && sect_node->type != DOLLY_ASM_NODE_SECTION_DATA) continue;

        dolly_asm_syntax_node* node
            = dolly_asm_syntax_tree_find(input, sect_node + 1,
                DOLLY_ASM_NODE_WRITABLE | DOLLY_ASM_NODE_SECTION);
        if (node == NULL) break;
        if (node->type & DOLLY_ASM_NODE_SECTION) continue;

        dolly_asm_syntax_node* last
            = dolly_asm_syntax_tree_find(input, node, DOLLY_ASM_NODE_SECTION);
        if (!last) {
            last = &input->nodes[input->size - 2];
        }

        last = dolly_asm_syntax_tree_rfind(input, last,
            DOLLY_ASM_NODE_WRITABLE);
        if (last == NULL) continue;
        ++last;

        uint32_t section_size = last->bin_offset - node->bin_offset;
        if (section_size == 0) continue;

        // NOTE: The "section numbers" generated in the semantic verification
        // phase are not necessarily correct since they do not ignore
        // empty sections (i.e. those with no writable data). Hence they are
        // overwritten here.
        // TODO: Cleaner approach
        for (dolly_asm_syntax_node* n = node; n != last; ++n) {
            n->section_number = output->header.section_count;
        }

        dolly_executable_section sect = {
            .type = sect_node->type == DOLLY_ASM_NODE_SECTION_TEXT
                                  ? DOLLY_SECTION_TEXT
                                  : DOLLY_SECTION_DATA,
            .size = section_size,
            .load_address = node->bin_offset
        };

        strlcpy(sect.name, sect_node->section_name,
                DOLLY_EXECUTABLE_SECTION_NAME_MAX_LENGTH - 1);
        dolly_executable_add_section(output, &sect, NULL);
    }

    // Second pass - assemble
    for (size_t index = 0; index < input->size; ++index) {
        dolly_asm_syntax_node* node = &input->nodes[index];
        dolly_executable_section* section =
            &output->sections[node->section_number];
        uint32_t write_pos = section->offset +
            (node->bin_offset - section->load_address);

        switch (node->type) {
        case DOLLY_ASM_NODE_INSTRUCTION: {
            dolly_opcode opcode = {
                .instr = node->instruction.instr,
                .a_mode = node->instruction.a_mode
            };
            output->program_data[write_pos] = dumb_opcode(opcode);
            uint16_t to_write;
            if (node->instruction.operand.is_identifier) {
                const tb_hash_node* entry
                    = tb_hash_table_get(&input->identifiers,
                        node->instruction.operand.identifier);
                const dolly_asm_syntax_node* iden_node
                    = &input->nodes[entry->int_value];
                if (iden_node->type == DOLLY_ASM_NODE_LABEL) {
                    if (dolly_is_branch(node->instruction.instr))
                        to_write
                            = (uint8_t)(iden_node->bin_offset
                                - node->bin_offset) - 2;
                    else to_write = iden_node->bin_offset;
                } else {
                    to_write = iden_node->identifier.addr_or_const;
                }
            } else {
                to_write = node->instruction.operand.integer;
            }
            output->program_data[write_pos + 1] = to_write & 0xFF;
            output->program_data[write_pos + 2] = (to_write & 0xFF00) >> 8;
            break;
        }
        case DOLLY_ASM_NODE_BYTE_DATA:
        case DOLLY_ASM_NODE_STRING: {
            const void* src
                = node->type == DOLLY_ASM_NODE_STRING ?
                    (const void*)node->identifier.name
                    : (const void*)node->directive.data;
            size_t len = node->type == DOLLY_ASM_NODE_STRING ?
                strlen(node->identifier.name)
                : node->directive.size;
            memcpy(output->program_data + write_pos, src, len);
            break;
        }
        case DOLLY_ASM_NODE_ORIGIN:
        case DOLLY_ASM_NODE_LABEL:
        case DOLLY_ASM_NODE_CONSTANT:
        default:
            break;
        }
    }
    return false;
}
