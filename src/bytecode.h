#ifndef func_lang_bytecode_h
#define func_lang_bytecode_h

#include "value.h"

enum Bytecode {
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
