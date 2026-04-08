#ifndef func_lang_bytecode_h
#define func_lang_bytecode_h

#include "value.h"

enum Bytecode {
    /// copy the value at the top of the stack to the given register
    OP_TRANSFER_STACK_REG,
    /// pushes the value in the given register to the stack
    OP_PUSH_REG_STACK,
    /// sets the given register to the value
    /// `op reg val`
    OP_SET_REG,

    /// pushes a u64 to the top of the stack
    OP_PUSH_U64,
    /// pops a u64 from the top of the stack
    OP_POP_U64,

    /// set the instruction pointer to the value at the given offset down from the stack top
    OP_JUMP_STACK,
    OP_JUMP_REG,

    /// adds or subtracts the given value from the instruction pointer
    /// `op i16`
    OP_JUMP_REL,

    /// boxes item at top of stack
    OP_BOX,
    /// stack: `[..args, arg_count, f]`
    OP_PARTIAL_APPLY,
    OP_CREATE_CLOSURE,

    OP_CALL,

    OP_PUSH_CONST,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_HEAD,
    OP_TAIL,

    OP_END,
};

enum Register {
    INSTRUCTION_PTR,
    STACK_PTR,
    REG_1,
    REG_COUNT,
};

struct Chunk {
    struct {
        u8 *ptr;
        u32 len;
        u32 cap;
    } bytecode;

    struct ValueList constants;
};

void chunk_write_byte(struct Chunk *chunk, u8 byte);
void chunk_add_constant(struct Chunk *chunk, struct Value value);

#endif
