#include "parse.h"

#include <stdlib.h>

dolly_asm_context dolly_asm_context_new(const char* filename)
{
    dolly_asm_context ctx = {
        .current_token_text = tb_stringbuf_new(32),
        .errors = 0,
        .filename = filename
    };

    if (ctx.current_token_text.data == NULL) abort_no_mem();

    return ctx;
}

void dolly_asm_context_destroy(dolly_asm_context* ctx)
{
    tb_stringbuf_destroy(&ctx->current_token_text);
}

const char* dolly_asm_token_type_str(const dolly_asm_token* token)
{
    switch (token->type) {
    case DOLLY_ASM_TOKEN_DIRECTIVE: return "directive";
    case DOLLY_ASM_TOKEN_INSTRUCTION: return "instruction";
    case DOLLY_ASM_TOKEN_IDENTIFIER: return "identifier";
    case DOLLY_ASM_TOKEN_INTEGER: return "integer";
    case DOLLY_ASM_TOKEN_STRING: return "string";
    case DOLLY_ASM_TOKEN_HASH: return "'#'";
    case DOLLY_ASM_TOKEN_COMMA: return "','";
    case DOLLY_ASM_TOKEN_OPEN_BRACKET: return "'('";
    case DOLLY_ASM_TOKEN_CLOSE_BRACKET: return "')'";
    case DOLLY_ASM_TOKEN_COLON: return "':'";
    case DOLLY_ASM_TOKEN_EQUALS: return "'='";
    case DOLLY_ASM_TOKEN_X: return "x-index";
    case DOLLY_ASM_TOKEN_Y: return "y-index";
    case DOLLY_ASM_TOKEN_A: return "accumulator operand";
    case DOLLY_ASM_TOKEN_ASTERISK: return "'*'";
    default: return "(unrecognised token)";
    }
}

void dolly_asm_report_error(dolly_asm_context* ctx)
{
    printf("%s:%zu:%zu: \x1b[1;31mError: \x1b[1;0m",
           ctx->filename, ctx->line,
           ctx->column - ctx->current_token_text.size - 1);
    ++ctx->errors;
}

void dolly_asm_report_error_token(dolly_asm_context* ctx,
                                         const dolly_asm_token* token)
{
    printf("%s:%zu:%zu: \x1b[1;31mError: \x1b[1;0m",
           ctx->filename, token->line, token->column);
    ++ctx->errors;
}

void dolly_asm_report_error_node(dolly_asm_context* ctx,
                                 const dolly_asm_syntax_node* node)
{
    printf("%s:%zu:%zu: \x1b[1;31mError: \x1b[1;0m",
           ctx->filename, node->line, node->column);
    ++ctx->errors;
}

dolly_asm_token_list dolly_asm_token_list_new(void)
{
    const static size_t START_CAPACITY = 32;
    dolly_asm_token_list list = {
        .tokens = malloc_or_abort(sizeof(dolly_asm_token) * START_CAPACITY),
        .size = 0,
        .capacity = START_CAPACITY
    };

    return list;
}

void dolly_asm_token_list_add(dolly_asm_token_list* list,
                              const dolly_asm_token* token)
{
    if (list->size >= list->capacity) {
        if (list->capacity == 0) ++list->capacity;
        else list->capacity *= 2;

        list->tokens = realloc_or_abort(list->tokens,
            sizeof(dolly_asm_token) * list->capacity);
    }

    list->tokens[list->size++] = *token;
}

void dolly_asm_token_list_destroy(dolly_asm_token_list* list)
{
    for (size_t i = 0; i < list->size; ++i) {
        if (list->tokens[i].type
            & (DOLLY_ASM_TOKEN_STRING | DOLLY_ASM_TOKEN_IDENTIFIER)) {
            free(list->tokens[i].string_or_identifier);
        }
    }
}

dolly_asm_syntax_tree dolly_asm_syntax_tree_new(void)
{
    const static size_t START_CAPACITY = 32;
    dolly_asm_syntax_tree tree = {
        .identifiers = tb_hash_table_new(67),
        .section_names = tb_hash_table_new(31),
        .nodes =
            malloc_or_abort(sizeof(dolly_asm_syntax_node) * START_CAPACITY),
        .size = 0,
        .capacity = START_CAPACITY
    };

    return tree;
}

void dolly_asm_syntax_tree_add(dolly_asm_syntax_tree* tree,
                               const dolly_asm_syntax_node* node)
{
    if (tree->size >= tree->capacity) {
        if (tree->capacity == 0) ++tree->capacity;
        else tree->capacity *= 2;

        tree->nodes = realloc_or_abort(tree->nodes,
            sizeof(dolly_asm_syntax_node) * tree->capacity);
    }

    tree->nodes[tree->size++] = *node;
}

dolly_asm_syntax_node*
dolly_asm_syntax_tree_find(dolly_asm_syntax_tree* tree,
                           dolly_asm_syntax_node* start,
                           dolly_asm_node_type type)
{
    for (dolly_asm_syntax_node* n = start;
         n != &tree->nodes[tree->size]; ++n) {
        if (n->type & type) return n;
    }

    return NULL;
}

dolly_asm_syntax_node*
dolly_asm_syntax_tree_rfind(dolly_asm_syntax_tree* tree,
                            dolly_asm_syntax_node* start,
                            dolly_asm_node_type type)
{
    for (dolly_asm_syntax_node* n = start; n != &tree->nodes[-1]; --n) {
        if (n->type & type) return n;
    }

    return NULL;
}

void dolly_asm_syntax_tree_destroy(dolly_asm_syntax_tree* tree)
{
    for (size_t i = 0; i < tree->size; ++i) {
        if (tree->nodes[i].type
            & (DOLLY_ASM_NODE_STRING | DOLLY_ASM_NODE_BYTE_DATA)) {
            free(tree->nodes[i].directive.data);
        }
    }

    tb_hash_table_destroy(&tree->identifiers);
    tb_hash_table_destroy(&tree->section_names);
}
