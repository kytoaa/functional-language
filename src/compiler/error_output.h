#ifndef func_lang_compiler_error_output_h
#define func_lang_compiler_error_output_h

#include "compiler.h"
#include "codegen/codegen.h"

void print_codegen_error(const struct CompilerConfig *config, struct CodegenError error);

#endif
