#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DOLLY_EXECUTABLE_VERSION 1
#define DOLLY_EXECUTABLE_SECTION_NAME_MAX_LENGTH 32

enum dolly_architecture
{
    DOLLY_ARCH_6502, DOLLY_ARCH_65816
};

typedef enum dolly_architecture dolly_architecture;

enum dolly_exec_section_type
{
    DOLLY_SECTION_TEXT, DOLLY_SECTION_DATA, DOLLY_SECTION_STRING
};

typedef enum dolly_exec_section_type dolly_exec_section_type;

struct dolly_executable_section
{
    char name[DOLLY_EXECUTABLE_SECTION_NAME_MAX_LENGTH];
    dolly_exec_section_type type;
    uint32_t offset, size, load_address;
};

typedef struct dolly_executable_section dolly_executable_section;

struct dolly_executable
{
    uint8_t* program_data;
    size_t   program_size; // Not serialised
    dolly_executable_section* sections;
    size_t sections_array_capacity; // Not serialised
    struct {
        dolly_architecture arch;
        uint8_t section_count;
        uint8_t version;
    } header;
};

typedef struct dolly_executable dolly_executable;

enum dolly_executable_status
{
    DOLLY_EXEC_OKAY, DOLLY_EXEC_INVALID_FORMAT, DOLLY_EXEC_INCOMPLETE_HEADER,
    DOLLY_EXEC_EOF_SECTION_TABLE, DOLLY_EXEC_EOF_SECTION
};

typedef enum dolly_executable_status dolly_executable_status;

void dolly_executable_init(dolly_executable* exec);
void dolly_executable_add_section(dolly_executable* exec,
                                  const dolly_executable_section* section,
                                  const uint8_t* data);
dolly_executable_status dolly_executable_read(dolly_executable* exec,
                                              const uint8_t* src,
                                              size_t size);
void dolly_executable_write(const dolly_executable* exec, FILE* file);
void dolly_executable_destroy(dolly_executable* exec);

const char* dolly_executable_error_msg(dolly_executable_status status);
const char* dolly_exec_sect_type_str(dolly_exec_section_type type);
