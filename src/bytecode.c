#include "bytecode.h"

void chunk_write_byte(struct Chunk *chunk, u8 byte)
{
    if (chunk->bytecode.len == chunk->bytecode.cap) {
        u32 new_cap = chunk->bytecode.cap == 0 ? 4 : chunk->bytecode.cap * 2;

        u8 *new_ptr = realloc_mem(chunk->bytecode.ptr, new_cap * sizeof(u8));
        chunk->bytecode.ptr = new_ptr;
        chunk->bytecode.cap = new_cap;
    }
    chunk->bytecode.ptr[chunk->bytecode.len++] = byte;
}

static const char *OP_NAMES[] = {
    [OP_NOOP] = "OP_NOOP",
    [OP_TRANSFER_STACK_REG] = "OP_TRANSFER_STACK_REG",
    [OP_PUSH_REG_STACK] = "OP_PUSH_REG_STACK",
    [OP_SET_REG] = "OP_SET_REG",
    [OP_SWAP] = "OP_SWAP",
    [OP_PUSH_U64] = "OP_PUSH_U64",
    [OP_POP_U64] = "OP_POP_U64",
    [OP_READ_BINDING] = "OP_READ_BINDING",
    [OP_CREATE_BINDING] = "OP_CREATE_BINDING",
    [OP_REMOVE_BINDING] = "OP_REMOVE_BINDING",
    [OP_JUMP] = "OP_JUMP",
    [OP_JUMP_STACK] = "OP_JUMP_STACK",
    [OP_JUMP_REG] = "OP_JUMP_REG",
    [OP_JUMP_REL] = "OP_JUMP_REL",
    [OP_JUMP_REL_CONDITIONAL] = "OP_JUMP_REL_CONDITIONAL",
    [OP_JUMP_GLOBALS] = "OP_JUMP_GLOBALS",
    [OP_CALL] = "OP_CALL",
    [OP_HANDLE_CONTINUATION] = "OP_HANDLE_CONTINUATION",
    [OP_EVAL] = "OP_EVAL",
    [OP_DYN_OBJ_READ] = "OP_DYN_OBJ_READ",
    [OP_CAPTURE_READ] = "OP_CAPTURE_READ",
    [OP_UPDATE_THUNK] = "OP_UPDATE_THUNK",
    [OP_PARTIAL_APPLY] = "OP_PARTIAL_APPLY",
    [OP_CREATE_CLOSURE] = "OP_CREATE_CLOSURE",
    [OP_WRITE_CLOSURE] = "OP_WRITE_CLOSURE",
    [OP_CREATE_THUNK] = "OP_CREATE_THUNK",
    [OP_WRITE_THUNK] = "OP_WRITE_THUNK",
    [OP_PUSH_CONST] = "OP_PUSH_CONST",
    [OP_TRUE] = "OP_TRUE",
    [OP_FALSE] = "OP_FALSE",
    [OP_EQUAL] = "OP_EQUAL",
    [OP_GREATER] = "OP_GREATER",
    [OP_LESS] = "OP_LESS",
    [OP_AND] = "OP_AND",
    [OP_OR] = "OP_OR",
    [OP_NOT] = "OP_NOT",
    [OP_ADD] = "OP_ADD",
    [OP_SUBTRACT] = "OP_SUBTRACT",
    [OP_MULTIPLY] = "OP_MULTIPLY",
    [OP_DIVIDE] = "OP_DIVIDE",
    [OP_HEAD] = "OP_HEAD",
    [OP_TAIL] = "OP_TAIL",
    [OP_END] = "OP_END",
};

const char *bytecode_op_name(enum Bytecode byte)
{
    if (byte > OP_END)
        return null;

    return OP_NAMES[byte];
}
