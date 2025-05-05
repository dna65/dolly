#include <stdio.h>

#include <errno.h>
#include <string.h>

#include "core/core.h"
#include "assembler/parse.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <input> [output]\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        printf("Failed to open file '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    tb_streambuf filebuf = tb_streambuf_new(128);
    tb_streambuf_read_all(&filebuf, file);
    fclose(file);

    if (!tb_streambuf_is_ok(&filebuf)) {
        printf("Failed to read file '%s': %s\n", argv[1], strerror(errno));
        tb_streambuf_destroy(&filebuf);
        return 1;
    }

    dolly_asm_context ctx = dolly_asm_context_new(argv[1]);

    dolly_asm_token_list tokens = dolly_asm_token_list_new();

    init_instruction_table();

    bool success = dolly_asm_lex(&ctx, filebuf.data, filebuf.size, &tokens);
    tb_streambuf_destroy(&filebuf);

    dolly_asm_syntax_tree syntax_tree = dolly_asm_syntax_tree_new();

    if (success)
        success = dolly_asm_make_syntax_tree(&ctx, &tokens, &syntax_tree);

    if (success)
        success = dolly_asm_verify_semantics(&ctx, &syntax_tree);

    if (ctx.errors) {
        printf("%zu errors generated.\n", ctx.errors);
        dolly_asm_syntax_tree_destroy(&syntax_tree);
        dolly_asm_token_list_destroy(&tokens);
        return 1;
    }

    dolly_executable exec;

    if (success)
        success = dolly_asm_make_executable(&syntax_tree, &exec);

    const char* out_filename = argc > 2 ? argv[2] : "out.bin";
    FILE* out = fopen(out_filename, "w");
    if (!out) {
        printf("Failed to open file for writing: %s\n", strerror(errno));
    } else {
        dolly_executable_write(&exec, out);
        printf("Assembled executable %s\n", out_filename);
    }

    dolly_asm_syntax_tree_destroy(&syntax_tree);
    dolly_executable_destroy(&exec);
    dolly_asm_token_list_destroy(&tokens);
    return success ? 0 : 1;
}
