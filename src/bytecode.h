#ifndef func_lang_bytecode_h
#define func_lang_bytecode_h

#include "value.h"

enum Bytecode {
    OP_NOOP,
    /// copy the value at the top of the stack to the given register
    OP_TRANSFER_STACK_REG,
    /// pushes the value in the given register to the stack
    OP_PUSH_REG_STACK,
    /// sets the given register to the value
    /// `op reg val`
    OP_SET_REG,

    /// swaps the two items at the top of the stack
    OP_SWAP,

    /// pushes a u64 to the top of the stack
    OP_PUSH_U64,
    /// pops a u64 from the top of the stack
    OP_POP_U64,

    /// unconditional jump, pops the address from the stack
    OP_JUMP,
    /// set the instruction pointer to the value at the given offset down from the stack top
    OP_JUMP_STACK,
    /// jump to the address in the register
    OP_JUMP_REG,

    /// adds or subtracts the given value from the instruction pointer
    /// `op i16`
    OP_JUMP_REL,

    /// jumps to a global function
    /// `op u8`
    OP_JUMP_GLOBALS,

    /// calls the function at the top of the stack
    OP_CALL,
    /// for handling application and calls with greater arguments than arity
    OP_HANDLE_CONTINUATION,
    /// evaluate the item at the top of the stack by calling its eval function
    OP_EVAL,

    /// reads the nth element in a thunk or closure
    /// `op u8`
    OP_DYN_OBJ_READ,

    OP_UPDATE_THUNK,

    /// `op u8` arg count
    /// stack: `[..args, f]`
    OP_PARTIAL_APPLY,
    /// `op u16` closure info index
    /// stack: `[..payload]`
    OP_CREATE_CLOSURE,

    OP_PUSH_CONST,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_AND,
    OP_OR,
    OP_NOT,
    /// add the items at the top of the stack, commutative
    OP_ADD,
    /// subtracts the items at the top of the stack
    /// expects `l` at top and `r` below for `l * r`
    OP_SUBTRACT,
    /// multiplies the items at the top of the stack, commutative
    OP_MULTIPLY,
    /// divides the items at the top of the stack
    /// expects `l` at top and `r` below for `l / r`
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
