#include "core/object.h"

#include "core/memory.h"

#include <stdlib.h>
#include <string.h>

const char DOLLY_MAGIC_NUMBER[] = { 0x7F, 'D', 'O', 'L', 'L', 'Y' };

void dolly_executable_init(dolly_executable* exec)
{
    memset(exec, 0, sizeof(dolly_executable));
    exec->header.version = DOLLY_EXECUTABLE_VERSION;
}

void dolly_executable_add_section(dolly_executable* exec,
                                  const dolly_executable_section* section,
                                  const uint8_t* data)
{
    if (exec->header.section_count >= exec->sections_array_capacity) {
        if (exec->sections_array_capacity == 0)
            exec->sections_array_capacity = 1;
        exec->sections_array_capacity *= 2;
        exec->sections = realloc_or_abort(exec->sections,
            sizeof(dolly_executable_section) * exec->sections_array_capacity);
    }

    uint32_t offset = 0;
    for (size_t i = 0; i < exec->header.section_count; ++i) {
        const dolly_executable_section* prev_section = &exec->sections[i];
        if (offset < prev_section->offset + prev_section->size) {
            offset = prev_section->offset + prev_section->size;
        }
    }

    memcpy(&exec->sections[exec->header.section_count++], section,
           sizeof(dolly_executable_section));
    exec->sections[exec->header.section_count - 1].offset = offset;

    exec->program_size += section->size;
    exec->program_data = realloc_or_abort(exec->program_data,
        exec->program_size);
    if (data != NULL) memcpy(exec->program_data + offset, data, section->size);
}

dolly_executable_status dolly_executable_read(dolly_executable* exec,
                                              const uint8_t* src,
                                              size_t size)
{
    const size_t HEADER_SIZE = sizeof(dolly_architecture)
                             + sizeof(uint8_t) + sizeof(uint8_t);

    if (size < HEADER_SIZE + sizeof(DOLLY_MAGIC_NUMBER))
        return DOLLY_EXEC_INCOMPLETE_HEADER;

    if (memcmp(src, DOLLY_MAGIC_NUMBER, sizeof(DOLLY_MAGIC_NUMBER)) != 0)
        return DOLLY_EXEC_INVALID_FORMAT;

    // Const is casted away as fmemopen has no const-qualified version
    FILE* buffer = fmemopen((uint8_t*) src + sizeof(DOLLY_MAGIC_NUMBER),
                            size - sizeof(DOLLY_MAGIC_NUMBER),
                            "r");
    // TODO: Endianness
    fread(&exec->header.arch, sizeof(dolly_architecture), 1, buffer);
    fread(&exec->header.version, sizeof(uint8_t), 1, buffer);
    fread(&exec->header.section_count, sizeof(uint8_t), 1, buffer);

    exec->sections_array_capacity = exec->header.section_count;

    if (exec->sections != NULL) free(exec->sections);
    exec->sections = malloc_or_abort(sizeof(dolly_executable_section)
                                     * exec->header.section_count);

    exec->program_size = 0;
    for (size_t i = 0; i < exec->header.section_count; ++i) {
        fread(exec->sections[i].name,
              DOLLY_EXECUTABLE_SECTION_NAME_MAX_LENGTH, 1, buffer);
        fread(&exec->sections[i].type, sizeof(dolly_exec_section_type), 1,
              buffer);
        fread(&exec->sections[i].offset, sizeof(uint32_t), 1, buffer);
        fread(&exec->sections[i].size, sizeof(uint32_t), 1, buffer);
        fread(&exec->sections[i].load_address, sizeof(uint32_t), 1, buffer);
        if (feof(buffer)) {
            fclose(buffer);
            return DOLLY_EXEC_EOF_SECTION_TABLE;
        }
        exec->program_size += exec->sections[i].size;
    }

    if (exec->program_data) free(exec->program_data);
    exec->program_data = malloc_or_abort((size_t) exec->program_size);

    for (size_t i = 0; i < exec->header.section_count; ++i) {
        fread(exec->program_data + exec->sections[i].offset,
              exec->sections[i].size, 1, buffer);
        if (feof(buffer)) {
            fclose(buffer);
            return DOLLY_EXEC_EOF_SECTION;
        }
    }

    fclose(buffer);
    return DOLLY_EXEC_OKAY;
}

void dolly_executable_write(const dolly_executable* exec, FILE* file)
{
    fwrite(DOLLY_MAGIC_NUMBER, sizeof(DOLLY_MAGIC_NUMBER), 1, file);
    fwrite(&exec->header.arch, sizeof(dolly_architecture), 1, file);
    fwrite(&exec->header.version, sizeof(uint8_t), 1, file);
    fwrite(&exec->header.section_count, sizeof(uint8_t), 1, file);

    for (size_t i = 0; i < exec->header.section_count; ++i) {
        const dolly_executable_section* section = &exec->sections[i];
        fwrite(section->name, DOLLY_EXECUTABLE_SECTION_NAME_MAX_LENGTH, 1,
               file);
        fwrite(&section->type, sizeof(dolly_exec_section_type), 1,
               file);
        fwrite(&section->offset, sizeof(uint32_t), 1, file);
        fwrite(&section->size, sizeof(uint32_t), 1, file);
        fwrite(&section->load_address, sizeof(uint32_t), 1, file);
    }

    fwrite(exec->program_data, exec->program_size, 1, file);
}

void dolly_executable_destroy(dolly_executable* exec)
{
    if (exec->sections != NULL) free(exec->sections);
    if (exec->program_data != NULL) free(exec->program_data);
}

const char* dolly_executable_error_msg(dolly_executable_status status)
{
    switch (status) {
    default: case DOLLY_EXEC_OKAY: return "";
    case DOLLY_EXEC_INVALID_FORMAT: return "not dolly executable";
    case DOLLY_EXEC_INCOMPLETE_HEADER: return "incomplete executable header";
    case DOLLY_EXEC_EOF_SECTION_TABLE: return "unexpected end of file"
                                              "in section table";
    case DOLLY_EXEC_EOF_SECTION: return "unexpected end of file in section";
    }
}

const char* dolly_exec_sect_type_str(dolly_exec_section_type type)
{
    switch (type) {
    case DOLLY_SECTION_TEXT: return "text";
    case DOLLY_SECTION_DATA: return "data";
    case DOLLY_SECTION_STRING: return "string";
    default: return "(unknown)";
    }
}
