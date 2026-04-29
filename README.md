# dolly

Dolly is a suite containing a 6502 virtual machine, assembler and disassembler.
It uses its own executable format "DOLLY".

## Build instructions

For POSIX-compliant systems, a shell script `build.sh` is provided, which
when executed will produce three executables: `dolly-asm`, `dolly-dsm` & `dolly-vm`.

A `CMakeLists.txt` is also provided for use with [CMake](https://cmake.org/).
From the root source directory:
```sh
cmake -B build
cmake --build build
```

An example "hello world" source file is included in `examples/`.
