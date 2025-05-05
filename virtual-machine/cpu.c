#include "virtual-machine/cpu.h"

#include "core/core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t dolly_cpu_resolve_operand_value(const dolly_cpu* cpu,
                                                const uint8_t* operand,
                                                dolly_addressing_mode a_mode,
                                                bool* page_crossed);

static uint8_t* dolly_cpu_resolve_operand_addr(dolly_cpu* cpu,
                                               const uint8_t* operand,
                                               dolly_addressing_mode a_mode);

static void dolly_cpu_update_flags_arithmetic(dolly_cpu* cpu, uint8_t value);

static void    dolly_cpu_stack_push(dolly_cpu* cpu, uint8_t value);
static uint8_t dolly_cpu_stack_pull(dolly_cpu* cpu);

static bool dolly_cpu_should_branch(const dolly_cpu* cpu,
                                    dolly_instruction branch);

void dolly_cpu_init(dolly_cpu* cpu)
{
    cpu->memory = malloc_or_abort(DOLLY_CPU_MEMORY_SIZE);
    memset(cpu->memory, 0, DOLLY_CPU_MEMORY_SIZE);
    cpu->reg_a = 0;
    cpu->reg_x = 0;
    cpu->reg_y = 0;
    cpu->stack_ptr = 0xFF;
    cpu->program_counter = 0;
}

void dolly_cpu_destroy(dolly_cpu* cpu)
{
    free(cpu->memory);
}

int dolly_cpu_read_next_instruction(dolly_cpu* cpu)
{
    int advance_by;
    int cycles
        = dolly_cpu_read_instruction(cpu, &cpu->memory[cpu->program_counter],
                                     &advance_by);
    cpu->program_counter += advance_by;
    return cycles;
}

static bool dolly_cpu_should_branch(const dolly_cpu* cpu, dolly_instruction branch)
{
    switch (branch) {
    case BPL: return cpu->flags.negative == false;
    case BMI: return cpu->flags.negative == true;
    case BVC: return cpu->flags.overflow == false;
    case BVS: return cpu->flags.overflow == true;
    case BCC: return cpu->flags.carry    == false;
    case BCS: return cpu->flags.carry    == true;
    case BNE: return cpu->flags.zero     == false;
    case BEQ: return cpu->flags.zero     == true;
    case BRA: return true;
    default:  return false;
    }
}

int dolly_cpu_read_instruction(dolly_cpu* cpu, const uint8_t* instruction,
                               int* advance_by)
{
    dolly_opcode op = dolly_resolve_opcode(*instruction);
    *advance_by = 1 + dolly_get_operand_size(op.a_mode);
    bool page_crossed;
    uint8_t* target_addr
        = dolly_cpu_resolve_operand_addr(cpu, instruction + 1, op.a_mode);
    uint16_t target_value
        = dolly_cpu_resolve_operand_value(cpu, instruction + 1, op.a_mode,
                                          &page_crossed);

    int8_t relative_target_value = (int8_t) target_value;
    bool page_crossed_branch
        = ((cpu->program_counter + relative_target_value) & 0xFF00)
          != (cpu->program_counter & 0xFF00);

    switch (op.instr) {
    /* FAMILY 1 */
    case LDA:
        cpu->reg_a = target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case STA:
        *target_addr = cpu->reg_a;
        return 2 + dolly_get_mode_cycles(op.a_mode, true);
    case ADC: {
        int result = (int)cpu->reg_a + target_value + cpu->flags.carry;
        cpu->flags.carry = result > 0xFF;
        cpu->flags.overflow
            = (cpu->reg_a ^ result) & (target_value ^ result) & 0x80;
        cpu->reg_a = result;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    }
    case SBC: {
        int result = (int)cpu->reg_a + (~target_value) + cpu->flags.carry;
        cpu->flags.carry = result < 0 ? 0 : 1;
        cpu->flags.overflow
            = (cpu->reg_a ^ result) & (target_value ^ result) & 0x80;
        cpu->reg_a = result;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    }
    case ORA:
        cpu->reg_a |= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case EOR:
        cpu->reg_a ^= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case AND:
        cpu->reg_a &= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case CMP:
        cpu->flags.carry = cpu->reg_a >= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a - target_value);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    /* FAMILY 2 */
    case ASL:
        cpu->flags.carry = *target_addr & 0x80;
        *target_addr <<= 1;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return (op.a_mode == ACCUMULATOR ? 2 : 4)
               + dolly_get_mode_cycles(op.a_mode, true);
    case ROL: {
        const int old_carry = cpu->flags.carry;
        cpu->flags.carry = *target_addr & 0x80;
        *target_addr <<= 1;
        *target_addr |= old_carry ? 1 : 0;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return (op.a_mode == ACCUMULATOR ? 2 : 4)
               + dolly_get_mode_cycles(op.a_mode, true);
    }
    case LSR: {
        cpu->flags.carry = *target_addr & 0x01;
        *target_addr >>= 1;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return (op.a_mode == ACCUMULATOR ? 2 : 4)
               + dolly_get_mode_cycles(op.a_mode, true);
    }
    case ROR: {
        const int old_carry = cpu->flags.carry;
        cpu->flags.carry = *target_addr & 0x01;
        *target_addr >>= 1;
        *target_addr |= old_carry ? 0x80 : 0;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return (op.a_mode == ACCUMULATOR ? 2 : 4)
               + dolly_get_mode_cycles(op.a_mode, true);
    }
    case STX:
        *target_addr = cpu->reg_x;
        return 2 + dolly_get_mode_cycles(op.a_mode, true);
    case LDX:
        cpu->reg_x = target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case DEC:
        --*target_addr;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return 4 + dolly_get_mode_cycles(op.a_mode, true);
    case INC:
        ++*target_addr;
        dolly_cpu_update_flags_arithmetic(cpu, *target_addr);
        return 4 + dolly_get_mode_cycles(op.a_mode, true);
    /* FAMILY 3 */
    case BIT:
        cpu->flags.zero = (cpu->reg_a & target_value) == 0;
        cpu->flags.overflow = target_value & 0x40;
        cpu->flags.negative = target_value & 0x80;
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case JMP:
        *advance_by = 0;
        cpu->program_counter = target_addr - cpu->memory;
        return op.a_mode == ABSOLUTE ? 3 : 5;
    case STY:
        *target_addr = cpu->reg_y;
        return 2 + dolly_get_mode_cycles(op.a_mode, true);
    case LDY:
        cpu->reg_y = target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_y);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case CPY:
        cpu->flags.carry = cpu->reg_y >= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_y - target_value);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    case CPX:
        cpu->flags.carry = cpu->reg_x >= target_value;
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x - target_value);
        return 2 + dolly_get_mode_cycles(op.a_mode, page_crossed);
    /* Branches */
    case BPL:
    case BMI:
    case BVC:
    case BVS:
    case BCC:
    case BCS:
    case BNE:
    case BEQ:
    case BRA:
        *advance_by = dolly_cpu_should_branch(cpu, op.instr)
                    ? relative_target_value + 2 : 2;
        return 2 + (*advance_by != 0 ? 1 : 0) + (page_crossed_branch ? 2 : 0);
    /* Interrupt-related */
    case BRK:
        // TODO: Check ordering
        dolly_cpu_stack_push(cpu, cpu->program_counter >> 8);
        dolly_cpu_stack_push(cpu, cpu->program_counter & 0x00FF);
        dolly_cpu_stack_push(cpu, cpu->flags_byte);
        cpu->program_counter = cpu->memory[0xFFFE];
        cpu->program_counter |= ((uint16_t)cpu->memory[0xFFFF] << 8);
        cpu->flags.break_flag = true;
        *advance_by = 0;
        return 7;
    case RTI:
        cpu->flags_byte = dolly_cpu_stack_pull(cpu);
        cpu->program_counter = dolly_cpu_stack_pull(cpu);
        cpu->program_counter |= ((uint16_t)dolly_cpu_stack_pull(cpu)) << 8;
        *advance_by = 1;
        return 6;
    /* Subroutine-related */
    case JSR:
        // JSR pushes the address of the next instruction minus one. On a real
        // 6502, the addition by one is deferred until the RTS instruction.
        cpu->program_counter += 2;
        dolly_cpu_stack_push(cpu, cpu->program_counter >> 8);
        dolly_cpu_stack_push(cpu, cpu->program_counter & 0x00FF);
        cpu->program_counter = target_addr - cpu->memory;
        *advance_by = 0;
        return 6;
    case RTS:
        cpu->program_counter = dolly_cpu_stack_pull(cpu);
        cpu->program_counter |= ((uint16_t)dolly_cpu_stack_pull(cpu)) << 8;
        *advance_by = 1;
        return 6;
    /* Stack */
    case PHP:
        dolly_cpu_stack_push(cpu, cpu->flags_byte);
        return 3;
    case PLP:
        cpu->flags_byte = dolly_cpu_stack_pull(cpu);
        return 4;
    case PHA:
        dolly_cpu_stack_push(cpu, cpu->reg_a);
        return 3;
    case PLA:
        cpu->reg_a = dolly_cpu_stack_pull(cpu);
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a);
        return 3;
    /* Increment/decrement X & Y */
    case INY:
        ++(cpu->reg_y);
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_y);
        return 2;
    case DEY:
        --(cpu->reg_y);
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_y);
        return 2;
    case INX:
        ++(cpu->reg_x);
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x);
        return 2;
    case DEX:
        --(cpu->reg_x);
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x);
        return 2;
    /* Transfer instructions */
    case TAY:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_y = cpu->reg_a);
        return 2;
    case TYA:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a = cpu->reg_y);
        return 2;
    case TAX:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x = cpu->reg_a);
        return 2;
    case TXA:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_a = cpu->reg_x);
        return 2;
    case TSX:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->reg_x = cpu->stack_ptr);
        return 2;
    case TXS:
        dolly_cpu_update_flags_arithmetic(cpu, cpu->stack_ptr = cpu->reg_x);
        return 2;
    case CLC:
        cpu->flags.carry = false;
        return 2;
    case SEC:
        cpu->flags.carry = true;
        return 2;
    case CLI:
        cpu->flags.interrupt_disable = false;
        return 2;
    case SEI:
        cpu->flags.interrupt_disable = true;
        return 2;
    case CLV:
        cpu->flags.overflow = false;
        return 2;
    case CLD:
        cpu->flags.decimal = false;
        return 2;
    case SED:
        cpu->flags.decimal = true;
        return 2;
    case NOP:
        return 2;
    default:
        fprintf(stderr, "Unrecognised instruction 0x%02x\n", *instruction);
        return -1;
    }
}

static void dolly_cpu_update_flags_arithmetic(dolly_cpu* cpu, uint8_t value)
{
    cpu->flags.zero = value == 0;
    cpu->flags.negative = value & 0x80;
}

static void dolly_cpu_stack_push(dolly_cpu* cpu, uint8_t value)
{
    cpu->memory[DOLLY_CPU_STACK_PAGE_OFFSET + cpu->stack_ptr--] = value;
}

static uint8_t dolly_cpu_stack_pull(dolly_cpu* cpu)
{
    return cpu->memory[DOLLY_CPU_STACK_PAGE_OFFSET + ++(cpu->stack_ptr)];
}

void dolly_cpu_debug(const dolly_cpu* cpu)
{
    printf("==========\n"
           "A = 0x%02x | X = 0x%02x | Y = 0x%02x\n"
           "SP = 0x%02x | PC = 0x%04x\n"
           "%c%c%c%c%c-%c%c\n"
           "==========\n",
           cpu->reg_a, cpu->reg_x, cpu->reg_y,
           cpu->stack_ptr, cpu->program_counter,
           cpu->flags.carry             ? 'C' : 'c',
           cpu->flags.zero              ? 'Z' : 'z',
           cpu->flags.interrupt_disable ? 'I' : 'i',
           cpu->flags.decimal           ? 'D' : 'd',
           cpu->flags.break_flag        ? 'B' : 'b',
           cpu->flags.overflow          ? 'O' : 'o',
           cpu->flags.negative          ? 'N' : 'n'
           );
}

static uint16_t dolly_cpu_resolve_operand_value(const dolly_cpu* cpu,
                                                const uint8_t* operand,
                                                dolly_addressing_mode a_mode,
                                                bool* page_crossed)
{
    uint16_t operand_16bits = (operand[1] << 8) + operand[0];

    switch (a_mode) {
    case IMMEDIATE:
    case RELATIVE:
        return *operand;
    case IMPLICIT:
        return 0;
    case ACCUMULATOR:
        return cpu->reg_a;
    case ZERO_PAGE:
        return cpu->memory[*operand];
    case ZERO_PAGE_X:
        return cpu->memory[(uint8_t)(*operand + cpu->reg_x)];
    case ZERO_PAGE_Y:
        return cpu->memory[(uint8_t)(*operand + cpu->reg_y)];
    case ABSOLUTE:
        return cpu->memory[operand_16bits];
    case ABSOLUTE_X:
        *page_crossed = (int)operand[0] + cpu->reg_x > 0xFF;
        return cpu->memory[operand_16bits + cpu->reg_x];
    case ABSOLUTE_Y:
         *page_crossed = (int)operand[0] + cpu->reg_y > 0xFF;
        return cpu->memory[operand_16bits + cpu->reg_y];
    case INDIRECT: {
        uint8_t lsb = cpu->memory[*operand];
        uint8_t msb = cpu->memory[(uint16_t)(*operand + 1)];
        uint16_t lo_addr = (msb << 8) + lsb;
        uint16_t hi_addr = lo_addr + 1;
        return (cpu->memory[hi_addr] << 8) + cpu->memory[lo_addr];
    }
    case INDIRECT_X: {
        uint8_t lsb = cpu->memory[(uint8_t)(*operand + cpu->reg_x)];
        uint8_t msb = cpu->memory[(uint8_t)(*operand + 1 + cpu->reg_x)];
        return cpu->memory[(msb << 8) + lsb];
    }
    case INDIRECT_Y: {
        uint8_t lsb = cpu->memory[operand[0]];
        uint8_t msb = cpu->memory[operand[0] + 1];
        uint16_t resolved_addr = (msb << 8) + (lsb + cpu->reg_y);
        *page_crossed = (int)lsb + cpu->reg_y > 0xFF;
        return cpu->memory[resolved_addr];
    }
    default: return 0;
    }
}

static uint8_t* dolly_cpu_resolve_operand_addr(dolly_cpu* cpu,
                                               const uint8_t* operand,
                                               dolly_addressing_mode a_mode)
{
    uint16_t operand_16bits = (operand[1] << 8) + operand[0];

    switch (a_mode) {
    default:
    case IMMEDIATE:
    case IMPLICIT:
    case RELATIVE:
        return NULL;
    case ACCUMULATOR:
        return &cpu->reg_a;
    case ZERO_PAGE:
        return &cpu->memory[*operand];
    case ZERO_PAGE_X:
        return &cpu->memory[(uint8_t)(*operand + cpu->reg_x)];
    case ZERO_PAGE_Y:
        return &cpu->memory[(uint8_t)(*operand + cpu->reg_y)];
    case ABSOLUTE:
        return &cpu->memory[operand_16bits];
    case ABSOLUTE_X:
        return &cpu->memory[operand_16bits + cpu->reg_x];
    case ABSOLUTE_Y:
        return &cpu->memory[operand_16bits + cpu->reg_y];
    case INDIRECT: {
        uint8_t lsb = cpu->memory[*operand];
        uint8_t msb = cpu->memory[(uint16_t)(*operand + 1)];
        return &cpu->memory[(msb << 8) + lsb];
    }
    case INDIRECT_X: {
        uint8_t lsb = cpu->memory[(uint16_t)(*operand + cpu->reg_x)];
        uint8_t msb = cpu->memory[(uint16_t)(*operand + cpu->reg_x + 1)];
        return &cpu->memory[(msb << 8) + lsb];
    }
    case INDIRECT_Y: {
        uint8_t lsb = cpu->memory[operand[0]];
        uint8_t msb = cpu->memory[(uint8_t)(operand[0] + 1)];
        return &cpu->memory[(msb << 8) + lsb + cpu->reg_y];
    }
    }
}
