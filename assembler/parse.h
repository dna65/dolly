#pragma once

#include <stddef.h>
#include <stdint.h>
#include "core/core.h"

enum dolly_asm_token_type
{
    // "Top level" tokens
    DOLLY_ASM_TOKEN_DIRECTIVE = 1 << 0,
    DOLLY_ASM_TOKEN_INSTRUCTION = 1 << 1,
    DOLLY_ASM_TOKEN_IDENTIFIER = 1 << 2,

    // The tokens below can only occur in the context of top level tokens
    DOLLY_ASM_TOKEN_INTEGER = 1 << 3,
    DOLLY_ASM_TOKEN_STRING = 1 << 4,
    DOLLY_ASM_TOKEN_HASH = 1 << 5,
    DOLLY_ASM_TOKEN_COMMA = 1 << 6,
    DOLLY_ASM_TOKEN_OPEN_BRACKET = 1 << 7,
    DOLLY_ASM_TOKEN_CLOSE_BRACKET = 1 << 8,
    DOLLY_ASM_TOKEN_COLON = 1 << 9,
    DOLLY_ASM_TOKEN_EQUALS = 1 << 10,
    DOLLY_ASM_TOKEN_X = 1 << 11,
    DOLLY_ASM_TOKEN_Y = 1 << 12,
    DOLLY_ASM_TOKEN_A = 1 << 13,
    DOLLY_ASM_TOKEN_ASTERISK = 1 << 14,
    DOLLY_ASM_TOKEN_NULL = 0 // Terminates token strings
};

enum dolly_asm_directive
{
    DOLLY_ASM_DIRECTIVE_ORIGIN,
    DOLLY_ASM_DIRECTIVE_BYTE,
    DOLLY_ASM_DIRECTIVE_STRING,
    DOLLY_ASM_DIRECTIVE_TEXT,
    DOLLY_ASM_DIRECTIVE_DATA,
    DOLLY_ASM_DIRECTIVE_WORD
};

enum dolly_asm_node_type
{
    DOLLY_ASM_NODE_SENTINEL = 0,
    DOLLY_ASM_NODE_INSTRUCTION = 1 << 0,
    DOLLY_ASM_NODE_LABEL = 1 << 1,
    DOLLY_ASM_NODE_CONSTANT = 1 << 2,
    DOLLY_ASM_NODE_STRING = 1 << 3,
    DOLLY_ASM_NODE_BYTE_DATA = 1 << 4,
    DOLLY_ASM_NODE_ORIGIN = 1 << 5,
    DOLLY_ASM_NODE_SECTION_TEXT = 1 << 6,
    DOLLY_ASM_NODE_SECTION_DATA = 1 << 7,
    DOLLY_ASM_NODE_WRITABLE = DOLLY_ASM_NODE_INSTRUCTION
                            | DOLLY_ASM_NODE_BYTE_DATA | DOLLY_ASM_NODE_STRING,
    DOLLY_ASM_NODE_SECTION = DOLLY_ASM_NODE_SECTION_TEXT
                           | DOLLY_ASM_NODE_SECTION_DATA

};

enum dolly_asm_operand_type
{
    DOLLY_ASM_OPERAND_INTEGER,
    DOLLY_ASM_OPERAND_IDENTIFIER,
    DOLLY_ASM_OPERAND_INTEGER_X,
    DOLLY_ASM_OPERAND_INTEGER_Y,
    DOLLY_ASM_OPERAND_IDENTIFIER_X,
    DOLLY_ASM_OPERAND_IDENTIFIER_Y,
    DOLLY_ASM_OPERAND_RELATIVE_INT,
    DOLLY_ASM_OPERAND_RELATIVE_IDEN,
    DOLLY_ASM_OPERAND_INDIRECT_INDEXED_INT,
    DOLLY_ASM_OPERAND_INDEXED_INDIRECT_INT,
    DOLLY_ASM_OPERAND_INDIRECT_INDEXED_IDEN,
    DOLLY_ASM_OPERAND_INDEXED_INDIRECT_IDEN,
    DOLLY_ASM_OPERAND_ACCUMULATOR,
    DOLLY_ASM_OPERAND_IMMEDIATE_INT,
    DOLLY_ASM_OPERAND_IMMEDIATE_IDEN,
    DOLLY_ASM_OPERAND_INDIRECT_INT,
    DOLLY_ASM_OPERAND_INDIRECT_IDEN,
    DOLLY_ASM_OPERAND_IMPLICIT
};

#define DOLLY_ASM_OPERAND_PATTERNS_COUNT 18

typedef enum dolly_asm_token_type dolly_asm_token_type;
typedef enum dolly_asm_directive dolly_asm_directive;
typedef enum dolly_asm_node_type dolly_asm_node_type;
typedef enum dolly_asm_operand_type dolly_asm_operand_type;

struct dolly_asm_token
{
    char* string_or_identifier;
    size_t line, column;
    union {
        dolly_asm_directive directive_type;
        dolly_instruction instruction;
        uint16_t integer_value;
    };
    dolly_asm_token_type type;
};

typedef struct dolly_asm_token dolly_asm_token;

struct dolly_asm_token_list
{
    dolly_asm_token* tokens;
    size_t size;
    size_t capacity;
};

typedef struct dolly_asm_token_list dolly_asm_token_list;

struct dolly_asm_identifier
{
    const char* name;
    uint16_t addr_or_const;
};

struct dolly_asm_data_directive
{
    dolly_asm_directive directive_type;
    uint8_t* data;
    size_t size;
};

struct dolly_asm_section_directive
{
    dolly_asm_directive directive_type;
    const char* sect_name;
};

struct dolly_asm_value
{
    union {
        uint16_t integer;
        const char* identifier;
    };
    bool is_identifier;
};

typedef struct dolly_asm_value dolly_asm_value;

struct dolly_asm_instruction
{
    dolly_asm_value operand;
    dolly_asm_operand_type operand_type;
    dolly_instruction instr;
    dolly_addressing_mode a_mode; // Only valid after semantic analysis
};

typedef struct dolly_asm_instruction dolly_asm_instruction;
typedef struct dolly_asm_identifier dolly_asm_identifier;
typedef struct dolly_asm_data_directive dolly_asm_data_directive;

struct dolly_asm_syntax_node
{
    size_t line, column;
    union {
        const char* section_name;
        dolly_asm_identifier identifier;
        dolly_asm_data_directive directive;
        dolly_asm_instruction instruction;
        uint16_t origin_offset;
    };
    dolly_asm_node_type type;
    uint32_t bin_offset;
    uint8_t section_number;
};

typedef struct dolly_asm_syntax_node dolly_asm_syntax_node;

struct dolly_asm_syntax_tree
{
    tb_hash_table identifiers;
    tb_hash_table section_names;
    dolly_asm_syntax_node* nodes;
    size_t size;
    size_t capacity;
};

typedef struct dolly_asm_syntax_tree dolly_asm_syntax_tree;

struct dolly_asm_context
{
    tb_stringbuf current_token_text;
    const char* filename;
    size_t errors;
    size_t line, column;
};

typedef struct dolly_asm_context dolly_asm_context;

void init_instruction_table(void);
dolly_asm_context dolly_asm_context_new(const char* filename);
void dolly_asm_context_destroy(dolly_asm_context* ctx);

/* Assembly stages */
bool dolly_asm_lex(dolly_asm_context* ctx, const char* text, size_t size,
                   dolly_asm_token_list* output);

bool dolly_asm_make_syntax_tree(dolly_asm_context* ctx,
                                const dolly_asm_token_list* input,
                                dolly_asm_syntax_tree* output);

bool dolly_asm_verify_semantics(dolly_asm_context* ctx,
                                dolly_asm_syntax_tree* input);

bool dolly_asm_make_executable(dolly_asm_syntax_tree* input,
                               dolly_executable* output);
/* =============== */

/* Error reporting */
void dolly_asm_report_error_token(dolly_asm_context* ctx,
                                  const dolly_asm_token* token);
void dolly_asm_report_error_node(dolly_asm_context* ctx,
                                 const dolly_asm_syntax_node* node);
void dolly_asm_report_error(dolly_asm_context* ctx);
/* =============== */

/* Dynamic arrays */
dolly_asm_token_list dolly_asm_token_list_new(void);
void dolly_asm_token_list_add(dolly_asm_token_list* list,
                              const dolly_asm_token* token);
void dolly_asm_token_list_destroy(dolly_asm_token_list* list);

dolly_asm_syntax_tree dolly_asm_syntax_tree_new(void);
void dolly_asm_syntax_tree_add(dolly_asm_syntax_tree* tree,
                               const dolly_asm_syntax_node* node);
dolly_asm_syntax_node*
dolly_asm_syntax_tree_find(dolly_asm_syntax_tree* tree,
                           dolly_asm_syntax_node* start,
                           dolly_asm_node_type type);
dolly_asm_syntax_node*
dolly_asm_syntax_tree_rfind(dolly_asm_syntax_tree* tree,
                            dolly_asm_syntax_node* start,
                            dolly_asm_node_type type);
void dolly_asm_syntax_tree_destroy(dolly_asm_syntax_tree* tree);
/* ============== */

/* Enum to string */
const char* dolly_asm_token_type_str(const dolly_asm_token* token);
/* ============== */
