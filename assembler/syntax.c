#include "assembler/parse.h"

#include "core/core.h"

/* Token matching */
static size_t dolly_asm_match_token(const dolly_asm_token_list* list,
                                    size_t start,
                                    dolly_asm_token_type match);

// Returns 0 if incomplete match, otherwise returns the number
// of tokens matched. Expects a null-terminated token string
static size_t dolly_asm_match_token_string(const dolly_asm_token_list* list,
                                           size_t start,
                                           const dolly_asm_token_type* str);
/* ============== */

static size_t dolly_asm_match_token(const dolly_asm_token_list* list,
                                    size_t start,
                                    dolly_asm_token_type match)
{
    size_t matched = 0;
    for (size_t index = start; index < list->size; ++index) {
        if ((list->tokens[index].type & match) == 0) {
            return matched;
        }
        ++matched;
    }

    return matched;
}

static size_t dolly_asm_match_token_string(const dolly_asm_token_list* list,
                                           size_t start,
                                           const dolly_asm_token_type* str)
{
    size_t matched = 0;
    for (size_t i = 0; i < (list->size - start); ++i) {
        if (str[i] == 0) return matched;
        if ((list->tokens[start + i].type & str[i]) == 0)
            return 0;
        ++matched;
    }

    return matched;
}

static void dolly_asm_read_identifier(dolly_asm_context* ctx,
                                      const dolly_asm_token_list* input,
                                      dolly_asm_syntax_tree* output,
                                      size_t* index)
{
    dolly_asm_token* token = &input->tokens[*index];

    const tb_hash_node* hash_entry;
    if ((hash_entry = tb_hash_table_get(&output->identifiers,
        token->string_or_identifier)) != NULL) {
        const dolly_asm_syntax_node* prev_def
            = &output->nodes[hash_entry->int_value];
        dolly_asm_report_error_token(ctx, token);
        printf("Duplicate symbol definition '%s'\n",
               token->string_or_identifier);
        printf("  (Previous definition in %s at line %zu:%zu)\n",
               ctx->filename, prev_def->line, prev_def->column);
        if (dolly_asm_match_token(input, *index + 1,
            DOLLY_ASM_TOKEN_EQUALS | DOLLY_ASM_TOKEN_COLON)) {
            *index += 1;
        }
        return;
    }

    if (dolly_asm_match_token(input, *index + 1, DOLLY_ASM_TOKEN_EQUALS)) {
        if (!dolly_asm_match_token(input, *index + 2,
            DOLLY_ASM_TOKEN_INTEGER)) {
            dolly_asm_report_error_token(ctx, token);
            printf("Expected integer for constant definition");
            if (*index + 2 < input->size - 1) {
                printf(", got '%s' instead\n",
                       dolly_asm_token_type_str(&input->tokens[*index + 2]));
            }
            putchar('\n');
            *index += 1;
            return;
        }
        tb_hash_table_add_int(&output->identifiers,
                              token->string_or_identifier,
                              (int)output->size);
        dolly_asm_syntax_node node = {
            .identifier = {
                .name = token->string_or_identifier,
                .addr_or_const = input->tokens[*index + 2].integer_value
            },
            .line = token->line,
            .column = token->column,
            .type = DOLLY_ASM_NODE_CONSTANT
        };

        dolly_asm_syntax_tree_add(output, &node);
        *index += 2;
    } else {
        if (dolly_asm_match_token(input, *index + 1, DOLLY_ASM_TOKEN_COLON))
            *index += 1;

        dolly_asm_syntax_node node = {
            .identifier = {
                .name = strdup_or_abort(token->string_or_identifier)
            },
            .line = token->line,
            .column = token->column,
            .type = DOLLY_ASM_NODE_LABEL
        };

        dolly_asm_syntax_tree_add(output, &node);
        tb_hash_table_add_int(&output->identifiers,
                              token->string_or_identifier,
                              (int)output->size - 1);
    }
}

static void dolly_asm_read_directive(dolly_asm_context* ctx,
                                     const dolly_asm_token_list* input,
                                     dolly_asm_syntax_tree* output,
                                     size_t* index)
{
    dolly_asm_token* token = &input->tokens[*index];
    dolly_asm_token* post_token = &input->tokens[*index + 1];

    switch (token->directive_type) {
    case DOLLY_ASM_DIRECTIVE_ORIGIN: {
        if (!dolly_asm_match_token(input, *index + 1,
            DOLLY_ASM_TOKEN_INTEGER)) {
            dolly_asm_report_error_token(ctx, post_token);
            printf("Expected address after .origin directive");
            if (*index + 1 < input->size - 1) {
                printf(", got %s instead",
                    dolly_asm_token_type_str(post_token));
            }
            putchar('\n');
            return;
        }

        dolly_asm_syntax_node node = {
            .origin_offset = post_token->integer_value,
            .line = token->line,
            .column = token->column,
            .type = DOLLY_ASM_NODE_ORIGIN
        };

        dolly_asm_syntax_tree_add(output, &node);
        *index += 1;
        break;
    }
    case DOLLY_ASM_DIRECTIVE_BYTE: {
        size_t matches
            = dolly_asm_match_token(input, *index + 1,
                DOLLY_ASM_TOKEN_INTEGER);
        if (matches < 1) {
            dolly_asm_report_error_token(ctx, post_token);
            printf("Expected at least one integer after .byte directive");
            if (*index + 1 < input->size - 1) {
                printf(", got %s instead",
                    dolly_asm_token_type_str(post_token));
            }
            putchar('\n');
            return;
        }

        for (size_t i = 0; i < matches; ++i) {
            int16_t val = input->tokens[*index + 1 + i].integer_value;
            if (val > UINT8_MAX || (int16_t)val < INT8_MIN) {
                dolly_asm_report_error_token(ctx, token);
                printf(".byte directive requires 8-bit integers\n");
                *index += i + 1;
                return;
            }
        }

        dolly_asm_syntax_node node = {
            .directive = {
                .directive_type = token->directive_type,
                .data = malloc_or_abort(matches * sizeof(uint8_t)),
                .size = matches
            },
            .line = token->line,
            .column = token->column,
            .type = DOLLY_ASM_NODE_BYTE_DATA
        };

        for (size_t i = 0; i < matches; ++i) {
            node.directive.data[i]
                = input->tokens[*index + 1 + i].integer_value;
        }

        dolly_asm_syntax_tree_add(output, &node);
        *index += matches;
        break;
    }
    case DOLLY_ASM_DIRECTIVE_STRING:
        if (!dolly_asm_match_token(input, *index + 1,
            DOLLY_ASM_TOKEN_STRING)) {
            dolly_asm_report_error_token(ctx, token);
            printf("Expected string after .string directive");
            if (*index + 1 < input->size - 1) {
                printf(", got %s instead",
                    dolly_asm_token_type_str(post_token));
            }
            putchar('\n');
            *index += 1;
            return;
        }
        dolly_asm_syntax_node node = {
            .identifier = {
                .name = post_token->string_or_identifier
            },
            .line = token->line,
            .column = token->column,
            .type = DOLLY_ASM_NODE_STRING
        };

        dolly_asm_syntax_tree_add(output, &node);
        *index += 1;
        break;
    case DOLLY_ASM_DIRECTIVE_TEXT:
    case DOLLY_ASM_DIRECTIVE_DATA: {
        if (!dolly_asm_match_token(input, *index + 1,
            DOLLY_ASM_TOKEN_STRING)) {
            dolly_asm_report_error_token(ctx, token);
            printf("Expected section name string after %s directive",
                token->directive_type == DOLLY_ASM_DIRECTIVE_TEXT
                    ? ".text"
                    : ".data");
            if (*index + 1 < input->size - 1) {
                printf(", got %s instead",
                    dolly_asm_token_type_str(post_token));
            }
            putchar('\n');
            *index += 1;
            break;
        }
        const tb_hash_node* hash_entry
            = tb_hash_table_get(&output->section_names,
                post_token->string_or_identifier);
        if (hash_entry != NULL) {
            const dolly_asm_syntax_node* prev_def
                = &output->nodes[hash_entry->int_value];
            dolly_asm_report_error_token(ctx, token);
            printf("Duplicate section name '%s'\n",
                post_token->string_or_identifier);
            printf("  (Previous definition in %s @ line %zu:%zu)\n",
                ctx->filename, prev_def->line, prev_def->column);
            *index += 1;
            break;
        }
        tb_hash_table_add_int(&output->section_names,
            post_token->string_or_identifier, (int)output->size);
        dolly_asm_syntax_node node = {
            .section_name = post_token->string_or_identifier,
            .line = token->line,
            .column = token->column,
            .type = token->directive_type == DOLLY_ASM_DIRECTIVE_TEXT ?
                    DOLLY_ASM_NODE_SECTION_TEXT : DOLLY_ASM_NODE_SECTION_DATA
        };
        dolly_asm_syntax_tree_add(output, &node);
        *index += 1;
        break;
    }
    }
}

const dolly_instruction IMPLIED_ONLY[25] = {
    BRK, CLC, CLD, CLI, CLV, DEY, DEX, INY, INX, NOP,
    PHP, PLA, PLP, RTI, RTS, SEC, SED, SEI, TAX, TAY,
    TSX, TYA, TXA, PHA, TXS
};

const dolly_asm_token_type
OPERAND_PATTERNS[DOLLY_ASM_OPERAND_PATTERNS_COUNT][6] = {
    [DOLLY_ASM_OPERAND_INTEGER] = { DOLLY_ASM_TOKEN_INTEGER, 0 },
    [DOLLY_ASM_OPERAND_IDENTIFIER] = { DOLLY_ASM_TOKEN_IDENTIFIER, 0 },
    [DOLLY_ASM_OPERAND_INTEGER_X] = {
        DOLLY_ASM_TOKEN_INTEGER, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_X, 0
    },
    [DOLLY_ASM_OPERAND_INTEGER_Y] = {
        DOLLY_ASM_TOKEN_INTEGER, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_Y, 0
    },
    [DOLLY_ASM_OPERAND_IDENTIFIER_X] = {
        DOLLY_ASM_TOKEN_IDENTIFIER, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_X, 0
    },
    [DOLLY_ASM_OPERAND_IDENTIFIER_Y] = {
        DOLLY_ASM_TOKEN_IDENTIFIER, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_Y, 0
    },
    [DOLLY_ASM_OPERAND_RELATIVE_INT] = {
        DOLLY_ASM_TOKEN_ASTERISK, DOLLY_ASM_TOKEN_INTEGER, 0
    },
    [DOLLY_ASM_OPERAND_RELATIVE_IDEN] = {
        DOLLY_ASM_TOKEN_ASTERISK, DOLLY_ASM_TOKEN_IDENTIFIER, 0
    },
    [DOLLY_ASM_OPERAND_INDIRECT_INDEXED_INT] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_INTEGER,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_Y, 0
    },
    [DOLLY_ASM_OPERAND_INDEXED_INDIRECT_INT] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_INTEGER,
        DOLLY_ASM_TOKEN_COMMA, DOLLY_ASM_TOKEN_X,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, 0
    },
    [DOLLY_ASM_OPERAND_INDIRECT_INDEXED_IDEN] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_IDENTIFIER,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, DOLLY_ASM_TOKEN_COMMA,
        DOLLY_ASM_TOKEN_Y, 0
    },
    [DOLLY_ASM_OPERAND_INDEXED_INDIRECT_IDEN] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_IDENTIFIER,
        DOLLY_ASM_TOKEN_COMMA, DOLLY_ASM_TOKEN_X,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, 0
    },
    [DOLLY_ASM_OPERAND_ACCUMULATOR] = {
        DOLLY_ASM_TOKEN_A, 0
    },
    [DOLLY_ASM_OPERAND_IMMEDIATE_INT] = {
        DOLLY_ASM_TOKEN_HASH, DOLLY_ASM_TOKEN_INTEGER, 0
    },
    [DOLLY_ASM_OPERAND_IMMEDIATE_IDEN] = {
        DOLLY_ASM_TOKEN_HASH, DOLLY_ASM_TOKEN_IDENTIFIER, 0
    },
    [DOLLY_ASM_OPERAND_INDIRECT_INT] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_INTEGER,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, 0
    },
    [DOLLY_ASM_OPERAND_INDIRECT_IDEN] = {
        DOLLY_ASM_TOKEN_OPEN_BRACKET, DOLLY_ASM_TOKEN_IDENTIFIER,
        DOLLY_ASM_TOKEN_CLOSE_BRACKET, 0
    },
    [DOLLY_ASM_OPERAND_IMPLICIT] = { 0 }
};

static void dolly_asm_read_instruction(dolly_asm_context* ctx,
                                       const dolly_asm_token_list* input,
                                       dolly_asm_syntax_tree* output,
                                       size_t* index)
{
    dolly_asm_token* token = &input->tokens[*index];

    dolly_asm_syntax_node node = {
        .instruction = {
            .instr = token->instruction
        },
        .line = token->line,
        .column = token->column,
        .type = DOLLY_ASM_NODE_INSTRUCTION
    };

    for (size_t i = 0; i < 25; ++i) {
        if (token->instruction == IMPLIED_ONLY[i]) {
            node.instruction.operand_type = DOLLY_ASM_OPERAND_IMPLICIT;
            dolly_asm_syntax_tree_add(output, &node);
            return;
        }
    }

    size_t largest_operand_index = 0;
    size_t largest_operand_size = 0;
    bool found_match = false;
    for (size_t i = 0; i < DOLLY_ASM_OPERAND_PATTERNS_COUNT; ++i) {
        size_t op_size
            = dolly_asm_match_token_string(input, *index + 1,
                OPERAND_PATTERNS[i]);
        if (op_size > largest_operand_size) {
            largest_operand_size = op_size;
            largest_operand_index = i;
            found_match = true;
        }
    }

    if (!found_match) {
        dolly_asm_report_error_token(ctx, &input->tokens[*index + 1]);
        printf("Invalid operand type\n");
        return;
    }

    switch ((dolly_asm_operand_type) largest_operand_index) {
    case DOLLY_ASM_OPERAND_INTEGER:
    case DOLLY_ASM_OPERAND_INTEGER_X:
    case DOLLY_ASM_OPERAND_INTEGER_Y:
        node.instruction.operand.integer
            = input->tokens[*index + 1].integer_value;
        node.instruction.operand.is_identifier = false;
        break;
    case DOLLY_ASM_OPERAND_IDENTIFIER:
    case DOLLY_ASM_OPERAND_IDENTIFIER_X:
    case DOLLY_ASM_OPERAND_IDENTIFIER_Y:
        node.instruction.operand.identifier
            = input->tokens[*index + 1].string_or_identifier;
        node.instruction.operand.is_identifier = true;
        break;
    case DOLLY_ASM_OPERAND_RELATIVE_INT:
    case DOLLY_ASM_OPERAND_INDIRECT_INDEXED_INT:
    case DOLLY_ASM_OPERAND_INDEXED_INDIRECT_INT:
    case DOLLY_ASM_OPERAND_IMMEDIATE_INT:
    case DOLLY_ASM_OPERAND_INDIRECT_INT:
        node.instruction.operand.integer
            = input->tokens[*index + 2].integer_value;
        node.instruction.operand.is_identifier = false;
        break;
    case DOLLY_ASM_OPERAND_RELATIVE_IDEN:
    case DOLLY_ASM_OPERAND_INDIRECT_INDEXED_IDEN:
    case DOLLY_ASM_OPERAND_INDEXED_INDIRECT_IDEN:
    case DOLLY_ASM_OPERAND_INDIRECT_IDEN:
    case DOLLY_ASM_OPERAND_IMMEDIATE_IDEN:
        node.instruction.operand.identifier
            = input->tokens[*index + 2].string_or_identifier;
        node.instruction.operand.is_identifier = true;
        break;
    default: break;
    }

    node.instruction.operand_type = largest_operand_index;

    dolly_asm_syntax_tree_add(output, &node);

    *index += largest_operand_size;
}

bool dolly_asm_make_syntax_tree(dolly_asm_context* ctx,
                                const dolly_asm_token_list* input,
                                dolly_asm_syntax_tree* output)
{
    dolly_asm_syntax_node default_section = {
        .section_name = "__default__",
        .line = 0,
        .column = 0,
        .type = DOLLY_ASM_NODE_SECTION_TEXT
    };
    dolly_asm_syntax_tree_add(output, &default_section);

    for (size_t index = 0; index < input->size; ++index) {
        dolly_asm_token* token = &input->tokens[index];
        switch (token->type) {
        case DOLLY_ASM_TOKEN_DIRECTIVE:
            dolly_asm_read_directive(ctx, input, output, &index);
            break;
        case DOLLY_ASM_TOKEN_INSTRUCTION:
            dolly_asm_read_instruction(ctx, input, output, &index);
            break;
        case DOLLY_ASM_TOKEN_IDENTIFIER:
            dolly_asm_read_identifier(ctx, input, output, &index);
            break;
        default:
            dolly_asm_report_error_token(ctx, token);
            printf("Unexpected %s\n", dolly_asm_token_type_str(token));
            break;
        }
    }

    dolly_asm_syntax_node sentinel = {
        .type = DOLLY_ASM_NODE_SENTINEL
    };
    dolly_asm_syntax_tree_add(output, &sentinel);

    return ctx->errors == 0;
}
