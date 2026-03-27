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

void chunk_add_constant(struct Chunk *chunk, struct Value value)
{
    if (chunk->constants.len == chunk->constants.cap) {
        u32 new_cap = chunk->constants.cap == 0 ? 4 : chunk->constants.cap * 2;

        struct Value *new_ptr = realloc_mem(chunk->constants.ptr, new_cap * sizeof(struct Value));
        chunk->constants.ptr = new_ptr;
        chunk->constants.cap = new_cap;
    }
    chunk->constants.ptr[chunk->constants.len++] = value;
}

