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

