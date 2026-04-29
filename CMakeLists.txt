cmake_minimum_required(VERSION 3.29)
project(dolly)

add_executable(dolly-vm)
target_sources(dolly-vm
  PRIVATE
    virtual-machine/main.c virtual-machine/cpu.c
    core/asm6502.c core/memory.c core/streambuf.c core/object.c

  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ./
    FILES core/core.h
)

add_executable(dolly-dsm)
target_sources(dolly-dsm
  PRIVATE
    disassembler/main.c disassembler/disassemble.c
    core/asm6502.c core/memory.c core/streambuf.c core/object.c

  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ./
    FILES core/core.h
)

add_executable(dolly-asm)
target_sources(dolly-asm
  PRIVATE
    assembler/main.c assembler/assemble.c assembler/lexer.c assembler/parse.c
    assembler/syntax.c assembler/semantics.c
    core/asm6502.c core/memory.c core/streambuf.c core/object.c
    core/stringbuf.c core/hash.c

  PRIVATE
    FILE_SET HEADERS
    BASE_DIRS ./
    FILES core/core.h
)
