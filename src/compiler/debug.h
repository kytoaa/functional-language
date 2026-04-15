#ifndef func_lang_compiler_debug_h
#define func_lang_compiler_debug_h

#include "../bytecode.h"
#include <stdio.h>

void print_instructions(FILE *output, const struct Chunk *chunk);
u32 print_instruction(FILE *out, u8 *bytes);

#endif
