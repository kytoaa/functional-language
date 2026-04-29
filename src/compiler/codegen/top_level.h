#ifndef func_lang_compiler_codegen_top_level_h
#define func_lang_compiler_codegen_top_level_h

#include "codegen.h"

struct CodegenErrorList generate_code(struct Compiler *compiler, struct ModuleCtx *modules);

#endif
