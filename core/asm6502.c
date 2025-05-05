#include "core/asm6502.h"

const dolly_instruction family_group_1[] = {
    ORA, AND, EOR, ADC, STA, LDA, CMP, SBC
};

const dolly_addressing_mode family_modes_1[] = {
    INDIRECT_X, ZERO_PAGE, IMMEDIATE, ABSOLUTE, INDIRECT_Y, ZERO_PAGE_X,
    ABSOLUTE_Y, ABSOLUTE_X
};

const dolly_instruction family_group_2[] = {
    ASL, ROL, LSR, ROR, STX, LDX, DEC, INC
};

const dolly_addressing_mode family_modes_2[] = {
    IMMEDIATE, ZERO_PAGE, ACCUMULATOR, ABSOLUTE, DOLLY_INVALID_ADDR_MODE,
    ZERO_PAGE_X, DOLLY_INVALID_ADDR_MODE, ABSOLUTE_X
};

const dolly_instruction family_group_3[] = {
    DOLLY_INVALID_INSTRUCTION, BIT, JMP, JMP /* absolute */, STY, LDY, CPY, CPX
};

const dolly_addressing_mode family_modes_3[] = {
    IMMEDIATE, ZERO_PAGE, DOLLY_INVALID_ADDR_MODE, ABSOLUTE,
    DOLLY_INVALID_ADDR_MODE, ZERO_PAGE_X, DOLLY_INVALID_ADDR_MODE, ABSOLUTE_X
};

const dolly_instruction branches[] = {
    BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
};

const dolly_instruction group_4[] = {
    PHP, CLC, PLP, SEC, PHA, CLI, PLA, SEI,
    DEY, TYA, TAY, CLV, INY, CLD, INX, SED
};

const dolly_instruction group_5[] = {
    TXA, TXS, TAX, TSX, DOLLY_INVALID_INSTRUCTION, DEX,
    DOLLY_INVALID_INSTRUCTION, NOP
};

dolly_opcode dolly_resolve_opcode(uint8_t opcode_byte)
{
    // 6502 opcode bits are usually in the pattern aaabbbcc
    // where aaa = opcode
    //       bbb = addressing mode
    //       cc  = family
    uint8_t family      =  opcode_byte & 0b00000011;
    uint8_t addr_mode   = (opcode_byte & 0b00011100) >> 2;
    uint8_t instr       = (opcode_byte & 0b11100000) >> 5;
    bool is_branch      = (opcode_byte & 0b00011111) == 0b00010000;
    // Group 4 encompasses INY, INX, DEY, DEX, TAY, TYA, the set/clear flag
    // and the push/pull stack instructions
    bool is_group_4     = (opcode_byte & 0b00001111) == 0b00001000;
    // Group 5 encompasses the rest of the transfer instructions, DEX and NOP
    bool is_group_5     = (opcode_byte & 0b10001010) == 0b10001010;

    dolly_opcode result = {
        .instr = DOLLY_INVALID_INSTRUCTION,
        .a_mode = DOLLY_INVALID_ADDR_MODE
    };

    if (is_branch) {
        result.instr = branches[instr];
        result.a_mode = RELATIVE;
        return result;
    }

    if (is_group_4) {
        result.instr = group_4[opcode_byte >> 4];
        result.a_mode = IMPLICIT;
        return result;
    }

    if (is_group_5) {
        result.instr = group_5[(opcode_byte >> 4) - 8];
        result.a_mode = IMPLICIT;
        return result;
    }

    switch (opcode_byte) {
    case 0x00:
        result.instr = BRK;
        result.a_mode = IMPLICIT;
        return result;
    case 0x20:
        result.instr = JSR;
        result.a_mode = ABSOLUTE;
        return result;
    case 0x40:
        result.instr = RTI;
        result.a_mode = IMPLICIT;
        return result;
    case 0x60:
        result.instr = RTS;
        result.a_mode = IMPLICIT;
        return result;
    default:
        break;
    }

    switch (family) {
    case 0b01:
        result.instr = family_group_1[instr];
        result.a_mode = family_modes_1[addr_mode];
        break;
    case 0b10:
        result.instr = family_group_2[instr];
        result.a_mode = family_modes_2[addr_mode];
        if (result.instr == LDX || result.instr == STX) {
            if (result.a_mode == ZERO_PAGE_X) result.a_mode = ZERO_PAGE_Y;
            if (result.a_mode == ABSOLUTE_X) result.a_mode = ABSOLUTE_Y;
        }
        break;
    case 0b00:
        result.instr = family_group_3[instr];
        if (result.instr == JMP)
            result.a_mode = opcode_byte == 0x4C ? ABSOLUTE : INDIRECT;
        else
            result.a_mode = family_modes_3[addr_mode];
        break;
    default:
        break;
    }

    return result;
}

int dolly_get_mode_cycles(dolly_addressing_mode a_mode, bool page_crossed)
{
    switch (a_mode) {
    case IMMEDIATE:
    case IMPLICIT:
    case ACCUMULATOR:
        return 0;
    case ZERO_PAGE:
        return 1;
    case ZERO_PAGE_X:
    case ZERO_PAGE_Y:
    case ABSOLUTE:
    case INDIRECT:
        return 2;
    case ABSOLUTE_X:
    case ABSOLUTE_Y:
        return 2 + (page_crossed ? 1 : 0);
    case INDIRECT_X:
        return 4;
    case INDIRECT_Y:
        return 3 + (page_crossed ? 1 : 0);
    case RELATIVE:
        return (page_crossed ? 2 : 0);
    }
    return -1;
}

int dolly_get_operand_size(dolly_addressing_mode a_mode)
{
    switch (a_mode) {
    case IMPLICIT:
    case ACCUMULATOR:
        return 0;
    case IMMEDIATE:
    case ZERO_PAGE:
    case ZERO_PAGE_X:
    case ZERO_PAGE_Y:
    case INDIRECT:
    case INDIRECT_X:
    case INDIRECT_Y:
    case RELATIVE:
        return 1;
    case ABSOLUTE:
    case ABSOLUTE_X:
    case ABSOLUTE_Y:
        return 2;
    }
    return -1;
}

const char* dolly_get_instr_name(dolly_instruction instr)
{
    switch (instr) {
    case LDA: return "LDA";
    case STA: return "STA";
    case ADC: return "ADC";
    case SBC: return "SBC";
    case ORA: return "ORA";
    case EOR: return "EOR";
    case AND: return "AND";
    case CMP: return "CMP";
    case ASL: return "ASL";
    case ROL: return "ROL";
    case LSR: return "LSR";
    case ROR: return "ROR";
    case STX: return "STX";
    case LDX: return "LDX";
    case DEC: return "DEC";
    case INC: return "INC";
    case BIT: return "BIT";
    case JMP: return "JMP";
    case STY: return "STY";
    case LDY: return "LDY";
    case CPY: return "CPY";
    case CPX: return "CPX";
    case BPL: return "BPL";
    case BMI: return "BMI";
    case BVC: return "BVC";
    case BVS: return "BVS";
    case BCC: return "BCC";
    case BCS: return "BCS";
    case BNE: return "BNE";
    case BEQ: return "BEQ";
    case BRK: return "BRK";
    case RTI: return "RTI";
    case JSR: return "JSR";
    case RTS: return "RTS";
    case PHP: return "PHP";
    case PLP: return "PLP";
    case PHA: return "PHA";
    case PLA: return "PLA";
    case INY: return "INY";
    case DEY: return "DEY";
    case INX: return "INX";
    case DEX: return "DEX";
    case TAY: return "TAY";
    case TYA: return "TYA";
    case TAX: return "TAX";
    case TXA: return "TXA";
    case TSX: return "TSX";
    case TXS: return "TXS";
    case CLC: return "CLC";
    case SEC: return "SEC";
    case CLI: return "CLI";
    case SEI: return "SEI";
    case CLV: return "CLV";
    case CLD: return "CLD";
    case SED: return "SED";
    case NOP: return "NOP";
    default:  return "~~~";
    }
}

const char* dolly_get_amode_name(dolly_addressing_mode a_mode)
{
    switch (a_mode) {
    case IMMEDIATE: return "immediate";
    case IMPLICIT: return "implicit";
    case ACCUMULATOR: return "accumulator";
    case ZERO_PAGE: return "zero-page";
    case ZERO_PAGE_X: return "zero-page X-indexed";
    case ZERO_PAGE_Y: return "zero-page Y-indexed";
    case ABSOLUTE: return "absolute";
    case ABSOLUTE_X: return "absolute X-indexed";
    case ABSOLUTE_Y: return "absolute Y-indexed";
    case INDIRECT: return "indirect";
    case INDIRECT_X: return "indexed indirect (X indirect)";
    case INDIRECT_Y: return "indirect indexed (Y indirect)";
    case RELATIVE: return "relative";
    default: return "(invalid addressing mode)";
    }
}

bool dolly_is_branch(dolly_instruction instr)
{
    switch (instr) {
    case BPL:
    case BMI:
    case BVC:
    case BVS:
    case BCC:
    case BCS:
    case BNE:
    case BEQ:
        return true;
    default:
        return false;
    }
}
