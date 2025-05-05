#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include "core/core.h"

#include "virtual-machine/cpu.h"
#include "virtual-machine/env.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s [options] <executable>\n", argv[0]);
        puts("Options:\n\t-d\tPrint debug information after execution");
        return 0;
    }

    bool print_debug_at_end = false;
    for (char* const* arg = &argv[0]; *arg; ++arg) {
        if (strcmp(*arg, "-d") == 0) print_debug_at_end = true;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        printf("Failed to open file '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    tb_streambuf file_buf = tb_streambuf_new(128);
    tb_streambuf_status sb_status = tb_streambuf_read_all(&file_buf, file);
    fclose(file);

    if (sb_status != TB_STREAMBUF_OKAY) {
        printf("Failed to read file '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    dolly_executable exec;
    dolly_executable_init(&exec);
    dolly_executable_status de_status
        = dolly_executable_read(&exec, (const uint8_t*) file_buf.data,
                                file_buf.size);
    tb_streambuf_destroy(&file_buf);

    if (de_status != DOLLY_EXEC_OKAY) {
        printf("Failed to read binary '%s': %s\n", argv[1],
               dolly_executable_error_msg(de_status));
        return 1;
    }

    dolly_cpu cpu;
    dolly_cpu_init(&cpu);

    bool found_start = false;

    for (uint8_t i = 0; i < exec.header.section_count; ++i) {
        dolly_executable_section* section = &exec.sections[i];
        if (section->load_address + section->size > DOLLY_CPU_MEMORY_SIZE) {
            continue;
        }
        memcpy(cpu.memory + section->load_address,
               exec.program_data + section->offset,
               section->size);
        if (strcmp(section->name, "_start") == 0
            && section->type == DOLLY_SECTION_TEXT) {
            cpu.program_counter = (uint16_t) section->load_address;
            found_start = true;
        }
    }

    dolly_executable_destroy(&exec);

    if (!found_start) {
        printf("Couldn't run executable: text section '_start' not found\n");
        dolly_cpu_destroy(&cpu);
        return 1;
    }

    int cycles = 0;
    bool run = true;
    while (run) {
        int delay = dolly_cpu_read_next_instruction(&cpu);
        if (delay == -1) break;
        if (cpu.flags.break_flag) {
            switch (cpu.reg_a) {
            case DOLLY_SYSCALL_EXIT:
                run = false;
                break;
            case DOLLY_SYSCALL_PRINT: {
                uint16_t print_vec = cpu.memory[0xFE]
                                   + ((uint16_t)cpu.memory[0xFF] << 8);
                printf("%s", (const char*)cpu.memory + print_vec);
                break;
            }
            default:
                puts("Invalid syscall, exiting");
                run = false;
                break;
            }
            cpu.flags.break_flag = 0;
        }
        cycles += delay;
    }

    if (print_debug_at_end) {
        printf("\n\nExecution done: %d cycles\nProcessor status:\n", cycles);
        dolly_cpu_debug(&cpu);
    }
    return 0;
}
