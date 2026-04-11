#include "../bytecode.h"
#include "../prelude.h"

/// basic calling conventions
/// `f x y z = ...`
/// stack: `[..cont x y z _]`
///                       ^ stack ptr

enum GlobalFunction {
    GLOBAL_ADD,
    GLOBAL_SUB,
    GLOBAL_MUL,
    GLOBAL_DIV,
    GLOBAL_EQUAL,
    GLOBAL_GREATER,
    GLOBAL_LESS,
    GLOBAL_GREATER_EQ,
    GLOBAL_LESS_EQ,
};

struct GlobalFunctionData {
    const u8 *code;
    const u32 length;
};

const struct GlobalFunctionData FUNCTIONS[];

/// expected stack layout
/// `[..cont l r]`
const u8 ADD_BYTECODE[] = {
    OP_EVAL, // [..cont l r']
    OP_SWAP, // [..cont r' l]
    OP_EVAL, // [..cont r' l']
    OP_ADD,  // [..cont res]
    OP_SWAP, // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 SUB_BYTECODE[] = {
    OP_EVAL,     // [..cont l r']
    OP_SWAP,     // [..cont r' l]
    OP_EVAL,     // [..cont r' l']
    OP_SUBTRACT, // [..cont res]
    OP_SWAP,     // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 MUL_BYTECODE[] = {
    OP_EVAL,     // [..cont l r']
    OP_SWAP,     // [..cont r' l]
    OP_EVAL,     // [..cont r' l']
    OP_MULTIPLY, // [..cont res]
    OP_SWAP,     // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 DIV_BYTECODE[] = {
    OP_EVAL,   // [..cont l r']
    OP_SWAP,   // [..cont r' l]
    OP_EVAL,   // [..cont r' l']
    OP_DIVIDE, // [..cont res]
    OP_SWAP,   // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 EQUAL_BYTECODE[] = {
    OP_EVAL,  // [..cont l r']
    OP_SWAP,  // [..cont r' l]
    OP_EVAL,  // [..cont r' l']
    OP_EQUAL, // [..cont res]
    OP_SWAP,  // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 LESS_BYTECODE[] = {
    OP_EVAL, // [..cont l r']
    OP_SWAP, // [..cont r' l]
    OP_EVAL, // [..cont r' l']
    OP_LESS, // [..cont res]
    OP_SWAP, // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 GREATER_BYTECODE[] = {
    OP_EVAL,    // [..cont l r']
    OP_SWAP,    // [..cont r' l]
    OP_EVAL,    // [..cont r' l']
    OP_GREATER, // [..cont res]
    OP_SWAP,    // [..res cont]
    OP_JUMP,
};

/// expected stack layout
/// `[..cont l r]`
const u8 GREATER_EQ_BYTECODE[] = {
    OP_PUSH_REG_STACK, (u8)INSTRUCTION_PTR,
    OP_JUMP_GLOBALS, (u8)GLOBAL_LESS,
    OP_NOT,
    OP_SWAP,
    OP_JUMP,
};
/// expected stack layout
/// `[..cont l r]`
const u8 LESS_EQ_BYTECODE[] = {
    OP_PUSH_REG_STACK, (u8)INSTRUCTION_PTR,
    OP_JUMP_GLOBALS, (u8)GLOBAL_GREATER,
    OP_NOT,
    OP_SWAP,
    OP_JUMP,
};

/// expected stack layout
/// `[..cont pair]`
const u8 HEAD_BYTECODE[] = {};

#define arr_len(arr) (sizeof(arr) / sizeof(arr[0]))

const struct GlobalFunctionData FUNCTIONS[] = {
    [GLOBAL_ADD]        = { ADD_BYTECODE,        arr_len(ADD_BYTECODE) },
    [GLOBAL_SUB]        = { SUB_BYTECODE,        arr_len(SUB_BYTECODE) },
    [GLOBAL_MUL]        = { MUL_BYTECODE,        arr_len(MUL_BYTECODE) },
    [GLOBAL_DIV]        = { DIV_BYTECODE,        arr_len(DIV_BYTECODE) },
    [GLOBAL_EQUAL]      = { EQUAL_BYTECODE,      arr_len(EQUAL_BYTECODE) },
    [GLOBAL_GREATER]    = { GREATER_BYTECODE,    arr_len(GREATER_BYTECODE) },
    [GLOBAL_LESS]       = { LESS_BYTECODE,       arr_len(LESS_BYTECODE) },
    [GLOBAL_GREATER_EQ] = { GREATER_EQ_BYTECODE, arr_len(GREATER_EQ_BYTECODE) },
    [GLOBAL_LESS_EQ]    = { LESS_EQ_BYTECODE,    arr_len(LESS_EQ_BYTECODE) },
};

