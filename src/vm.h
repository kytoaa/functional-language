#ifndef func_lang_vm_h
#define func_lang_vm_h

#include "value.h"
#include "object.h"

#define STACK_SIZE 256
#define FRAMES_MAX 64

struct CallFrame {
    struct Function *function;
    u8 *ip;
    struct Value *slots;
};

struct VM {
    struct CallFrame frames[FRAMES_MAX];
    u32 frame_count;
    u32 stack_height;
    struct Value stack[STACK_SIZE];
};

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
};

extern struct VM vm;

#endif
