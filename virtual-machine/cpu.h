#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DOLLY_CPU_STACK_PAGE_OFFSET 0x0100
#define DOLLY_CPU_MEMORY_SIZE       0x10000

struct dolly_cpu
{
    uint8_t* memory;
    uint8_t  reg_a, reg_x, reg_y;
    uint8_t  stack_ptr;
    uint16_t program_counter;
    union
    {
        struct
        {
            bool carry             : 1;
            bool zero              : 1;
            bool interrupt_disable : 1;
            bool decimal           : 1;
            bool break_flag        : 1;
            bool empty_flag        : 1;
            bool overflow          : 1;
            bool negative          : 1;
        } flags;
        uint8_t flags_byte;
    };
};

typedef struct dolly_cpu dolly_cpu;

void dolly_cpu_init(dolly_cpu* cpu);
void dolly_cpu_destroy(dolly_cpu* cpu);

// Returns the number of cycles taken, -1 if invalid instruction
int dolly_cpu_read_next_instruction(dolly_cpu* cpu);
int dolly_cpu_read_instruction(dolly_cpu* cpu, const uint8_t* instruction,
                               int* advance_by);

void dolly_cpu_debug(const dolly_cpu* cpu);

