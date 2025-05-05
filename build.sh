COMPILE_FLAGS="-I. -Wall -Werror"
CC=clang

# Virtual machine
echo "Building virtual machine..." &&
$CC   virtual-machine/main.c virtual-machine/cpu.c \
      core/asm6502.c core/memory.c core/streambuf.c \
      core/object.c $COMPILE_FLAGS -o dolly-vm &&

# Disassembler
echo "Building disassembler..." &&
$CC   disassembler/main.c disassembler/disassemble.c \
      core/asm6502.c core/memory.c core/streambuf.c \
      core/object.c $COMPILE_FLAGS -o dolly-dsm &&

# Assembler
echo "Building assembler..." &&
$CC   assembler/main.c assembler/assemble.c \
      assembler/lexer.c assembler/parse.c assembler/syntax.c \
      assembler/semantics.c \
      core/asm6502.c core/memory.c core/streambuf.c \
      core/object.c core/stringbuf.c core/hash.c \
      $COMPILE_FLAGS -o dolly-asm
