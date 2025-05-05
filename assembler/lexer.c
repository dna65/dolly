#include "parse.h"

#include <ctype.h>
#include <string.h>

/* Multi-character lexing */
static bool dolly_asm_parse_string(dolly_asm_context* ctx,
                                   const char* start, size_t end,
                                   dolly_asm_token* out,
                                   size_t* index);
static bool dolly_asm_parse_multichar(dolly_asm_context* ctx,
                                      dolly_asm_token* out);
/* ====================== */

static void dolly_asm_delimit_token(dolly_asm_context* ctx,
                                    dolly_asm_token_list* token_list);

/* Strings */
static int strcmp_ignorecase(const char* a, const char* b);
/* ======= */

tb_hash_table INSTRUCTION_TABLE;

static int strcmp_ignorecase(const char* a, const char* b)
{
    size_t i;
    for (i = 0; a[i] && b[i]; ++i) {
        if (toupper(a[i]) != toupper(b[i])) break;
    }

    return toupper(a[i]) - toupper(b[i]);
}

void init_instruction_table(void)
{
    INSTRUCTION_TABLE = tb_hash_table_new(67);

    tb_hash_table_add_int(&INSTRUCTION_TABLE, "ADC", ADC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "AND", AND);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "ASL", ASL);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BCC", BCC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BCS", BCS);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BEQ", BEQ);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BIT", BIT);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BMI", BMI);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BNE", BNE);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BPL", BPL);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BRK", BRK);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BVC", BVC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "BVS", BVS);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CLC", CLC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CLI", CLI);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CLV", CLV);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CMP", CMP);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CPX", CPX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CPY", CPY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "DEC", DEC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "DEX", DEX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "DEY", DEY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "EOR", EOR);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "INC", INC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "INX", INX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "INY", INY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "JMP", JMP);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "JSR", JSR);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "LDX", LDX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "LDY", LDY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "LSR", LSR);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "NOP", NOP);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "ORA", ORA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "PHA", PHA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "PHP", PHP);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "PLA", PLA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "PLP", PLP);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "ROL", ROL);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "ROR", ROR);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "RTI", RTI);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "RTS", RTS);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "SBC", SBC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "SED", SED);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "SEI", SEI);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "STA", STA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "STX", STX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "STY", STY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TAX", TAX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TAY", TAY);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TSX", TSX);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TXA", TXA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TXS", TXS);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "TYA", TYA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "SEC", SEC);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "LDA", LDA);
    tb_hash_table_add_int(&INSTRUCTION_TABLE, "CLD", CLD);
}

static bool dolly_asm_parse_string(dolly_asm_context* ctx,
                                   const char* start, size_t end,
                                   dolly_asm_token* out, size_t* index)
{
    out->type = DOLLY_ASM_TOKEN_STRING;
    out->line = ctx->line;
    out->column = ctx->column;
    size_t i = 1;
    for (i = 1; start[i] != '"'; ++i) {
        ++ctx->column;
        if (!isprint(start[i])) {
            dolly_asm_report_error(ctx);
            printf("Illegal character in string\n");
            tb_stringbuf_clear(&ctx->current_token_text);
            return false;
        }

        if (start[i] == '\\') {
            if (i + 1 >= end) {
                dolly_asm_report_error(ctx);
                printf("Unexpected end of string\n");
                tb_stringbuf_clear(&ctx->current_token_text);
                return false;
            }
            char to_cat;
            switch (start[i + 1]) {
            case 'n': to_cat = '\n'; break;
            case 't': to_cat = '\t'; break;
            case '\\': to_cat = '\\'; break;
            case '"': to_cat = '"'; break;
            default:
                dolly_asm_report_error(ctx);
                printf("Unrecognised escape character\n");
                tb_stringbuf_clear(&ctx->current_token_text);
                return false;
            }
            tb_stringbuf_cat_char(&ctx->current_token_text, to_cat);
            ++i;
        } else {
            tb_stringbuf_cat_char(&ctx->current_token_text, start[i]);
        }
    }
    *index += i;
    out->string_or_identifier = strdup(ctx->current_token_text.data);
    tb_stringbuf_clear(&ctx->current_token_text);
    return true;
}

// Returns false if conversion stopped prematurely
static bool dec_str_to_int(const char* str, int* output)
{
    *output = 0;
    size_t i = 0;
    if (str[0] == '-' || str[0] == '+') ++i;
    for (; str[i]; ++i) {
       if (!isdigit(str[i])) return false;
        *output *= 10;
        *output += (str[i] - '0');
    }
    if (str[0] == '-') *output *= -1;

    return i > 0;
}

static bool hex_str_to_int(const char* str, int* output)
{
    *output = 0;
    size_t i = 0;
    if (str[0] == '-' || str[0] == '+') ++i;
    for (; str[i]; ++i) {
        int lowercase = tolower(str[i]);
        if (!isdigit(str[i]) && (lowercase < 'a' || lowercase > 'f'))
            return false;
        *output *= 16;
        *output += lowercase - (isdigit(lowercase) ? '0' : 'a' - 10);
    }
    if (str[0] == '-') *output *= -1;

    return i > 0;
}

static bool dolly_asm_parse_multichar(dolly_asm_context* ctx,
                                      dolly_asm_token* out)
{
    // Directive matching
    out->line = ctx->line;
    out->column = ctx->column - ctx->current_token_text.size;
    size_t text_len = ctx->current_token_text.size;
    const char* text = ctx->current_token_text.data;
    if (text[0] == '.') {
        if (strcmp_ignorecase(text, ".ORG") == 0) {
            out->directive_type = DOLLY_ASM_DIRECTIVE_ORIGIN;
        } else if (strcmp_ignorecase(text, ".BYTE") == 0) {
            out->directive_type = DOLLY_ASM_DIRECTIVE_BYTE;
        } else if (strcmp_ignorecase(text, ".STRING") == 0) {
            out->directive_type = DOLLY_ASM_DIRECTIVE_STRING;
        } else if (strcmp_ignorecase(text, ".TEXT") == 0) {
            out->directive_type = DOLLY_ASM_DIRECTIVE_TEXT;
        } else if (strcmp_ignorecase(text, ".DATA") == 0) {
            out->directive_type = DOLLY_ASM_DIRECTIVE_DATA;
        } else {
            dolly_asm_report_error(ctx);
            printf("Unrecognised or unsupported directive '%s'\n", text);
            return false;
        }
        out->type = DOLLY_ASM_TOKEN_DIRECTIVE;
        return true;
    }

    // Match instruction
    if (ctx->current_token_text.size == 3) {
        char instruction_text[4] = {0};
        memcpy(instruction_text, text, 4);
        for (size_t i = 0; i < 4; ++i)
            instruction_text[i] = toupper(instruction_text[i]);

        const tb_hash_node* node
            = tb_hash_table_get(&INSTRUCTION_TABLE, instruction_text);
        if (node != NULL) {
            out->type = DOLLY_ASM_TOKEN_INSTRUCTION;
            out->instruction = (dolly_instruction) node->int_value;
            return true;
        }
    }

    // TODO: Match operators & integers in a more sophisticated manner
    // Match integer
    if (text[0] == '$' || isdigit(text[0]) || text[0] == '-'
        || text[0] == '+') {
        int items_read;
        int value;
        if (text[0] == '$')
            items_read = hex_str_to_int(text + 1, &value);
        else
            items_read = dec_str_to_int(text, &value);

        if (items_read > 0) {
            if (value > UINT16_MAX || value < INT16_MIN) {
                dolly_asm_report_error(ctx);
                printf("Integer overflow: '%s'\n", text);
                return false;
            }
            out->type = DOLLY_ASM_TOKEN_INTEGER;
            out->integer_value = (uint16_t) value;
            return true;
        }
    }

    if (strcmp_ignorecase(text, "x") == 0) {
        out->type = DOLLY_ASM_TOKEN_X;
        return true;
    } else if (strcmp_ignorecase(text, "y") == 0) {
        out->type = DOLLY_ASM_TOKEN_Y;
        return true;
    } else if (strcmp_ignorecase(text, "a") == 0) {
        out->type = DOLLY_ASM_TOKEN_A;
        return true;
    }

    // Match identifier
    bool valid_identifier = true;
    for (size_t i = 0; i < text_len; ++i) {
        if (i == 0 && !isalpha(text[i]) && text[0] != '_') {
            valid_identifier = false;
            break;
        }
        if (!isalnum(text[i]) && text[i] != '_') {
            valid_identifier = false;
            break;
        }
    }

    if (valid_identifier) {
        out->type = DOLLY_ASM_TOKEN_IDENTIFIER;
        out->string_or_identifier = strdup_or_abort(text);
        return true;
    }

    dolly_asm_report_error(ctx);
    printf("Unrecognised token '%s'\n", text);

    return false;
}

static void dolly_asm_delimit_token(dolly_asm_context* ctx,
                                    dolly_asm_token_list* token_list)
{
    if (ctx->current_token_text.size == 0) return;
    dolly_asm_token token;
    if (dolly_asm_parse_multichar(ctx, &token))
        dolly_asm_token_list_add(token_list, &token);
    tb_stringbuf_clear(&ctx->current_token_text);
}

bool dolly_asm_lex(dolly_asm_context* ctx, const char* text, size_t size,
                   dolly_asm_token_list* output)
{
    ctx->line = 1;
    ctx->column = 1;
    dolly_asm_token base_token = {
        .line = ctx->line,
        .column = ctx->column
    };
    for (size_t index = 0; index < size; ++index) {
        switch (text[index]) {
        // Single character self-delimiting tokens
        case '(':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_OPEN_BRACKET;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case ')':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_CLOSE_BRACKET;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case '#':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_HASH;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case ':':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_COLON;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case '=':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_EQUALS;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case ',':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_COMMA;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case '*':
            dolly_asm_delimit_token(ctx, output);
            base_token.type = DOLLY_ASM_TOKEN_ASTERISK;
            dolly_asm_token_list_add(output, &base_token);
            break;
        case ';':
            dolly_asm_delimit_token(ctx, output);
            while (text[index] != '\n' && index < size) {
                ++index;
            }
            ++ctx->line;
            ctx->column = 0;
            break;
        // Whitespace
        case '\n':
        case '\r':
        case '\t':
        case ' ': {
            dolly_asm_delimit_token(ctx, output);
            if (text[index] == '\n') {
                ctx->column = 0;
                ++ctx->line;
            }
            break;
        }
        // String quotes
        case '"': {
            dolly_asm_delimit_token(ctx, output);
            dolly_asm_token token;
            if (dolly_asm_parse_string(ctx, text + index,
                                       size - index, &token, &index)) {
                dolly_asm_token_list_add(output, &token);
            }
            break;
        }
        // Multi-character tokens
        default:
            tb_stringbuf_cat_char(&ctx->current_token_text, text[index]);
            break;
        }
        ++ctx->column;
    }

    return ctx->errors == 0;
}
