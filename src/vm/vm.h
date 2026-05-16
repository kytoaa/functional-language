#ifndef func_lang_vm_h
#define func_lang_vm_h

#include "../value.h"
#include "../object.h"
#include "../bytecode.h"
#include <stdio.h>

#define STACK_SIZE 256
#define IDENT_COUNT 256

struct Code {
    u64 *constants;
    struct ClosureInfo *functions;
    u8 *instructions;
    u64 global_function_start;
};

struct VmConfig {
    FILE *out;
    FILE *error;
};

struct VM {
    struct VmConfig config;
    struct Code code;
    /// `instruction ptr - points to next byte`
    /// `stack ptr - index of next free stack slot`
    u64 registers[REG_COUNT];
    u64 bindings[IDENT_COUNT];
    u64 stack[STACK_SIZE];
    struct {
        struct Thunk **ptr;
        u32 len;
    } static_thunks;
    bool had_error;
};

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
};

extern struct VM vm;

void run_vm(struct Chunk *chunk, struct VmConfig config);
void end_vm();

#endif
