#include "disassembler/disassemble.h"

#include "core/core.h"

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s <input>\n", argv[0]);
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

    dolly_executable exec;
    memset(&exec, 0, sizeof(dolly_executable));

    dolly_executable_status de_status
        = dolly_executable_read(&exec, (const uint8_t*) filebuf.data,
                                filebuf.size);
    tb_streambuf_destroy(&filebuf);

    if (de_status != DOLLY_EXEC_OKAY) {
        printf("Error whilst reading executable: %s\n",
               dolly_executable_error_msg(de_status));
        dolly_executable_destroy(&exec);
        return 1;
    }

    for (size_t i = 0; i < exec.header.section_count; ++i) {
        const dolly_executable_section* section = &exec.sections[i];
        // TODO: Implement disassembly of data sections
        if (section->type != DOLLY_SECTION_TEXT) continue;
        dolly_dsm_list dsm = dolly_dsm_list_new();
        dolly_dsm_status status
            = dolly_dsm_list_read(&dsm, exec.program_data + section->offset,
                                  section->size);
        if (status != DOLLY_DSM_OKAY) {
            printf("Error whilst disassembling: %s\n", dolly_dsm_error_msg(status));
            return 1;
        }
        printf("== Section: %s, Name: %s, Load address: 0x%x, Offset: 0x%x ==\n",
               dolly_exec_sect_type_str(section->type), section->name,
               section->load_address, section->offset);
        dolly_dsm_list_write(&dsm, stdout, section->offset);
        dolly_dsm_list_destroy(&dsm);
    }
    dolly_executable_destroy(&exec);

    return 0;
}
