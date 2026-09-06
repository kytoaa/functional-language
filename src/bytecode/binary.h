#ifndef func_lang_bytecode_binary_h
#define func_lang_bytecode_binary_h

#include "stdio.h"

#include "../bytecode.h"

enum WriteChunkResult {
    WRITE_CHUNK_ERR = false,
    WRITE_CHUNK_OK = true,
};

enum WriteChunkResult write_chunk_to(FILE *out, const struct Chunk *chunk);
enum WriteChunkResult chunk_from(u8 *bytes, usize len, struct Chunk *out);

#endif
