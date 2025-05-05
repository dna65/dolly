#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DOLLY_INVALID_ADDR_MODE 0
#define DOLLY_INVALID_INSTRUCTION -1

#define DOLLY_6502_INSTRUCTION_COUNT 57

enum dolly_addressing_mode
{
    IMMEDIATE = 1 << 0,
    IMPLICIT = 1 << 1,
    ACCUMULATOR = 1 << 2,
    ZERO_PAGE = 1 << 3,
    ZERO_PAGE_X = 1 << 4,
    ZERO_PAGE_Y = 1 << 5,
    ABSOLUTE = 1 << 6,
    ABSOLUTE_X = 1 << 7,
    ABSOLUTE_Y = 1 << 8,
    INDIRECT = 1 << 9,
    INDIRECT_X = 1 << 10,
    INDIRECT_Y = 1 << 11,
    RELATIVE = 1 << 12
};

typedef enum dolly_addressing_mode dolly_addressing_mode;

enum dolly_instruction
{
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC,
    CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP, JSR,
    LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI, RTS, SBC,
    SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA, SEC, LDA, CLD,
    BRA
};

typedef enum dolly_instruction dolly_instruction;

struct dolly_opcode
{
    dolly_instruction instr;
    dolly_addressing_mode a_mode;
};

typedef struct dolly_opcode dolly_opcode;

dolly_opcode dolly_resolve_opcode(uint8_t opcode);
int dolly_get_mode_cycles(dolly_addressing_mode a_mode, bool page_crossed);
int dolly_get_operand_size(dolly_addressing_mode a_mode);
const char* dolly_get_instr_name(dolly_instruction instr);
const char* dolly_get_amode_name(dolly_addressing_mode a_mode);
bool dolly_is_branch(dolly_instruction instr);
