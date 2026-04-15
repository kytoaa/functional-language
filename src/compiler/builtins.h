#ifndef func_lang_compiler_builtins_h
#define func_lang_compiler_builtins_h

#include "../prelude.h"

enum GlobalFunction {
    GLOBAL_FUNC_ADD,
    GLOBAL_FUNC_SUB,
    GLOBAL_FUNC_MUL,
    GLOBAL_FUNC_DIV,
    GLOBAL_FUNC_EQUAL,
    GLOBAL_FUNC_GREATER,
    GLOBAL_FUNC_LESS,
    GLOBAL_FUNC_GREATER_EQ,
    GLOBAL_FUNC_LESS_EQ,
    GLOBAL_FUNC_APPL_CONT,
    GLOBAL_FUNCTION_COUNT,
};

u32 global_functions_size();
/// `code` must have capacity given by `global_functions_size()`
void write_global_functions(u8 *code);
u32 global_function_offset(enum GlobalFunction function);

#endif
