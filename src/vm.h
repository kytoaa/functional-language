#ifndef func_lang_vm_h
#define func_lang_vm_h

#include "value.h"
#include "object.h"
#include "bytecode.h"
#include <stdio.h>

#define STACK_SIZE 256

struct Code {
    u8 *instructions;
};

struct VmConfig {
    FILE *out;
    FILE *error;
};

struct VM {
    struct VmConfig config;
    struct Code *code;
    /// `instruction ptr - points to next byte`
    /// `stack ptr - index of next free stack slot`
    u64 registers[REG_COUNT];
    u64 stack[STACK_SIZE];
};

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
};

extern struct VM vm;

#endif
