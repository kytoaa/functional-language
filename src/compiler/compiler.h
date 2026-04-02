#ifndef func_lang_compiler_h
#define func_lang_compiler_h

#include "../prelude.h"
#include <stdio.h>

struct CompilerConfig {
    FILE *output;
    FILE *error;
    const char *src;
    const char *file_name;
    u32 file_name_len;
};

void compile_file(const struct CompilerConfig *config);

#endif
