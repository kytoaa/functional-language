#ifndef func_lang_vm_h
#define func_lang_vm_h

#include "../value.h"
#include "../object.h"
#include "../bytecode.h"
#include <stdio.h>

#define STACK_SIZE 1024
#define IDENT_COUNT 1024

struct Code {
    u64 *constants;
    struct ClosureInfo *functions;
    struct TypeInfo *types;
    u8 *instructions;
    u64 global_function_start;
    u16 type_count;
};

struct VmConfig {
    FILE *out;
    FILE *error;
};

struct VM {
    struct VmConfig config;
    struct Code code;
    struct {
        struct Thunk **ptr;
        u32 len;
    } static_thunks;
    bool had_error;
    /// `instruction ptr - points to next byte`
    /// `stack ptr - index of next free stack slot`
    u64 registers[REG_COUNT];
    usize stack_len;
    usize bindings_len;
    u64 *bindings;
    u64 *stack;
};

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
};

extern struct VM vm;

void run_vm(struct Chunk *chunk, struct VmConfig config);
void end_vm();

#endif
