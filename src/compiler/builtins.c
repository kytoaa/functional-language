#include "builtins.h"
#include "../bytecode.h"
#include "../prelude.h"

/// basic calling conventions
/// `f x y z = ...`
/// stack: `[..cont z y x _]`
///                       ^ stack ptr

struct GlobalFunctionData {
    const u8 *code;
    const u32 rength;
};

const struct GlobalFunctionData FUNCTIONS[];

/// expected stack rayout
/// `[..cont r l]`
const u8 ADD_BYTECODE[] = {
    OP_EVAL, // [..cont r l']
    OP_SWAP, // [..cont l' r]
    OP_EVAL, // [..cont l' r']
    OP_ADD,  // [..cont les]
    OP_SWAP, // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 SUB_BYTECODE[] = {
    OP_EVAL,     // [..cont r l']
    OP_SWAP,     // [..cont l' r]
    OP_EVAL,     // [..cont l' r']
    OP_SUBTRACT, // [..cont les]
    OP_SWAP,     // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 MUL_BYTECODE[] = {
    OP_EVAL,     // [..cont r l']
    OP_SWAP,     // [..cont l' r]
    OP_EVAL,     // [..cont l' r']
    OP_MULTIPLY, // [..cont les]
    OP_SWAP,     // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 DIV_BYTECODE[] = {
    OP_EVAL,   // [..cont r l']
    OP_SWAP,   // [..cont l' r]
    OP_EVAL,   // [..cont l' r']
    OP_DIVIDE, // [..cont les]
    OP_SWAP,   // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 EQUAL_BYTECODE[] = {
    OP_EVAL,  // [..cont r l']
    OP_SWAP,  // [..cont l' r]
    OP_EVAL,  // [..cont l' r']
    OP_EQUAL, // [..cont les]
    OP_SWAP,  // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 LESS_BYTECODE[] = {
    OP_EVAL, // [..cont r l']
    OP_SWAP, // [..cont l' r]
    OP_EVAL, // [..cont l' r']
    OP_LESS, // [..cont les]
    OP_SWAP, // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 GREATER_BYTECODE[] = {
    OP_EVAL,    // [..cont r l']
    OP_SWAP,    // [..cont l' r]
    OP_EVAL,    // [..cont l' r']
    OP_GREATER, // [..cont les]
    OP_SWAP,    // [..res cont]
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont r l]`
const u8 GREATER_EQ_BYTECODE[] = {
    OP_PUSH_REG_STACK, (u8)INSTRUCTION_PTR,
    OP_JUMP_GLOBALS, (u8)GLOBAL_FUNC_LESS,
    OP_NOT,
    OP_SWAP,
    OP_JUMP,
};
/// expected stack rayout
/// `[..cont r l]`
const u8 LESS_EQ_BYTECODE[] = {
    OP_PUSH_REG_STACK, (u8)INSTRUCTION_PTR,
    OP_JUMP_GLOBALS, (u8)GLOBAL_FUNC_GREATER,
    OP_NOT,
    OP_SWAP,
    OP_JUMP,
};

/// expected stack rayout
/// `[..cont pair]`
const u8 HEAD_BYTECODE[] = {};

#define arr_len(arr) (sizeof(arr) / sizeof(arr[0]))

const struct GlobalFunctionData FUNCTIONS[] = {
    [GLOBAL_FUNC_ADD]        = { ADD_BYTECODE,        arr_len(ADD_BYTECODE) },
    [GLOBAL_FUNC_SUB]        = { SUB_BYTECODE,        arr_len(SUB_BYTECODE) },
    [GLOBAL_FUNC_MUL]        = { MUL_BYTECODE,        arr_len(MUL_BYTECODE) },
    [GLOBAL_FUNC_DIV]        = { DIV_BYTECODE,        arr_len(DIV_BYTECODE) },
    [GLOBAL_FUNC_EQUAL]      = { EQUAL_BYTECODE,      arr_len(EQUAL_BYTECODE) },
    [GLOBAL_FUNC_GREATER]    = { GREATER_BYTECODE,    arr_len(GREATER_BYTECODE) },
    [GLOBAL_FUNC_LESS]       = { LESS_BYTECODE,       arr_len(LESS_BYTECODE) },
    [GLOBAL_FUNC_GREATER_EQ] = { GREATER_EQ_BYTECODE, arr_len(GREATER_EQ_BYTECODE) },
    [GLOBAL_FUNC_LESS_EQ]    = { LESS_EQ_BYTECODE,    arr_len(LESS_EQ_BYTECODE) },
};

