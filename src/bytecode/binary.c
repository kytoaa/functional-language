#include "binary.h"
#include "../compiler_info.h"

#include <string.h>

#define HEADER FILE_EXTENSION "-bytecode"

enum WriteChunkResult write_chunk_to(FILE *out, const struct Chunk *chunk)
{
    // write header
    usize written = fwrite(HEADER, sizeof(HEADER), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    // write bytecode len
    written = fwrite(&chunk->bytecode.len, sizeof(chunk->bytecode.len), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    // write bytecode
    written = fwrite(
        chunk->bytecode.ptr,
        sizeof(*chunk->bytecode.ptr),
        chunk->bytecode.len,
        out
    );
    if (written < chunk->bytecode.len) {
        return WRITE_CHUNK_ERR;
    }

    // write constants len
    written = fwrite(&chunk->constants.len, sizeof(chunk->constants.len), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    // write constants
    written = fwrite(
        chunk->constants.ptr,
        sizeof(*chunk->constants.ptr),
        chunk->constants.len,
        out
    );
    if (written < chunk->constants.len) {
        return WRITE_CHUNK_ERR;
    }

    // write closures len
    written = fwrite(&chunk->closures.len, sizeof(chunk->closures.len), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    // write closures
    written = fwrite(
        chunk->closures.ptr,
        sizeof(*chunk->closures.ptr),
        chunk->closures.len,
        out
    );
    if (written < chunk->closures.len) {
        return WRITE_CHUNK_ERR;
    }

    // write types len
    written = fwrite(&chunk->types.len, sizeof(chunk->types.len), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    // write types
    for (u32 i = 0; i < chunk->types.len; i++) {
        struct TypeInfo info = chunk->types.ptr[i];
        info.name = null;

        written = fwrite(&info, sizeof(info), 1, out);
        if (written < 1) {
            return WRITE_CHUNK_ERR;
        }
    }

    // write type names
    for (u32 i = 0; i < chunk->types.len; i++) {
        struct TypeInfo info = chunk->types.ptr[i];
        written = fwrite(info.name, info.name_len, 1, out);

        if (written < 1) {
            return WRITE_CHUNK_ERR;
        }
    }

    // write strings len
    written = fwrite(&chunk->strings.len, sizeof(chunk->strings.len), 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    written = fwrite(chunk->strings.ptr, chunk->strings.len, 1, out);
    if (written < 1) {
        return WRITE_CHUNK_ERR;
    }

    return WRITE_CHUNK_OK;
}

enum WriteChunkResult chunk_from(u8 *bytes, usize len, struct Chunk *out)
{
    usize position = 0;
    struct Chunk chunk = {};
    init_chunk(&chunk);

    // read header
    {
        usize l = sizeof(HEADER);
        usize eq = memcmp(bytes, HEADER, l);
        if (eq != 0) {
            goto error;
        }
        position += l;
    }

    // read bytecode len
    {
        usize l = sizeof(chunk.bytecode.len);
        if (l + position > len) {
            goto error;
        }
        memcpy(&chunk.bytecode.len, &bytes[position], l);
        position += l;
    }
    // read bytecode
    {
        usize l = sizeof(*chunk.bytecode.ptr) * chunk.bytecode.len;
        if (l + position > len) {
            chunk.bytecode.len = 0;
            goto error;
        }
        chunk.bytecode.cap = chunk.bytecode.len;
        chunk.bytecode.ptr = alloc_mem(l);

        memcpy(chunk.bytecode.ptr, &bytes[position], l);
        position += l;
    }

    usize prev_const_len = chunk.constants.len;
    // read constants len
    {
        usize l = sizeof(chunk.constants.len);
        if (l + position > len) {
            goto error;
        }
        memcpy(&chunk.constants.len, &bytes[position], l);
        position += l;
    }
    // read constants
    {
        usize l = sizeof(*chunk.constants.ptr) * chunk.constants.len;
        if (l + position > len) {
            chunk.constants.len = 0;
            goto error;
        }
        chunk.constants.cap = chunk.constants.len;
        chunk.constants.ptr = realloc_mem(chunk.constants.ptr, l);

        usize existing = sizeof(*chunk.constants.ptr) * prev_const_len;

        memcpy(
            chunk.constants.ptr + prev_const_len,
            &bytes[position + existing],
            l - existing
        );
        position += l;
    }

    // read closures len
    {
        usize l = sizeof(chunk.closures.len);
        if (l + position > len) {
            goto error;
        }
        memcpy(&chunk.closures.len, &bytes[position], l);
        position += l;
    }
    // read closures
    {
        usize l = sizeof(*chunk.closures.ptr) * chunk.closures.len;
        if (l + position > len) {
            chunk.closures.len = 0;
            goto error;
        }
        chunk.closures.cap = chunk.closures.len;
        chunk.closures.ptr = alloc_mem(l);

        memcpy(chunk.closures.ptr, &bytes[position], l);
        position += l;
    }

    // read types len
    {
        usize l = sizeof(chunk.types.len);
        if (l + position > len) {
            goto error;
        }
        memcpy(&chunk.types.len, &bytes[position], l);
        position += l;
    }
    // read types
    {
        usize l = sizeof(*chunk.types.ptr) * chunk.types.len;
        if (l + position > len) {
            chunk.types.len = 0;
            goto error;
        }
        chunk.types.cap = chunk.types.len;
        chunk.types.ptr = realloc_mem(chunk.types.ptr, l);

        usize existing = sizeof(*chunk.types.ptr) * BUILTIN_TYPE_COUNT;

        memcpy(
            chunk.types.ptr + BUILTIN_TYPE_COUNT,
            &bytes[position + existing],
            l - existing
        );
        position += l;
    }

    // read type names
    for (u32 i = BUILTIN_TYPE_COUNT; i < chunk.types.len; i++) {
        struct TypeInfo *type = &chunk.types.ptr[i];

        usize l = sizeof(char) * type->name_len;
        if (l + position > len) {
            goto error;
        }

        type->name = alloc_mem(l);

        memcpy(type->name, &bytes[position], l);
        position += l;
    }

    // read strings len
    {
        usize l = sizeof(chunk.strings.len);
        if (l + position > len) {
            goto error;
        }
        memcpy(&chunk.strings.len, &bytes[position], l);
        position += l;
    }
    // read strings
    {
        usize l = sizeof(*chunk.strings.ptr) * chunk.strings.len;
        if (l + position > len) {
            chunk.strings.len = 0;
            goto error;
        }
        chunk.strings.cap = chunk.strings.len;
        chunk.strings.ptr = alloc_mem(l);

        memcpy(chunk.strings.ptr, &bytes[position], l);
        position += l;
    }

    *out = chunk;

    return WRITE_CHUNK_OK;

error:
    free_chunk(&chunk);
    return WRITE_CHUNK_ERR;
}
