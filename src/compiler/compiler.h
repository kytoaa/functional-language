#ifndef func_lang_compiler_h
#define func_lang_compiler_h

#include "../prelude.h"
#include "../bytecode.h"
#include <stdio.h>

struct LibraryPath {
    const char *path;
    const char *name;
    u32 path_len;
    u32 name_len;
};

struct CompilerConfig {
    FILE *output;
    FILE *error;
    const char *file_name;
    struct LibraryPath *libraries;
    u32 file_name_len;
    u32 library_count;
};

struct FileData {
    const char *src;
    const char *file_name;
    u32 file_name_len;
};

bool compile_file(const struct CompilerConfig config, struct Chunk *out);
bool load_bytecode_file(const struct CompilerConfig config, struct Chunk *out);
bool save_bytecode_file(const struct CompilerConfig config, const struct Chunk *chunk);

#endif
