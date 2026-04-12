#include "codegen.h"
#include "../../prelude.h"
#include "../../bytecode.h"
#include "../../object.h"

void declare_ident(struct Context *ctx, const char *ident)
{
    if (ctx->ident_stack_len == IDENT_STACK_SIZE) {
        panic("more than 256 identifiers");
    }
    ctx->ident_stack[ctx->ident_stack_len] = (struct Identifier){
        .ident = ident,
        .function = ctx->compiling_chunk->closures.len - 1,
        .depth = ctx->current_depth,
    };
    ctx->ident_stack_len += 1;
}

struct IdentSearchResult get_ident_offset(struct Context *ctx, const char *ident) 
{
    for (u16 i = 0; i < ctx->ident_stack_len; i++) {
        struct Identifier *searching_ident = &ctx->ident_stack[ctx->ident_stack_len - (i + 1)];
        if (searching_ident->ident == ident) {
            return (struct IdentSearchResult){ .offset = i, .function = searching_ident->function };
        }
    }
    return (struct IdentSearchResult){ UINT16_MAX, 0 };
}

void emit_byte(struct Context *ctx, u8 byte)
{
    chunk_write_byte(ctx->compiling_chunk, byte);
}
void emit_2_bytes(struct Context *ctx, u8 byte, u8 arg)
{
    chunk_write_byte(ctx->compiling_chunk, byte);
    chunk_write_byte(ctx->compiling_chunk, arg);
}
void emit_u16(struct Context *ctx, u16 value)
{
    union {
        u8 bytes[2];
        u16 val;
    } val = { .val = value };
    emit_byte(ctx, val.bytes[0]);
    emit_byte(ctx, val.bytes[1]);
}
void emit_u32(struct Context *ctx, u32 value)
{
    union {
        u16 bytes[2];
        u32 val;
    } val = { .val = value };
    emit_u16(ctx, val.bytes[0]);
    emit_u16(ctx, val.bytes[1]);
}
u32 get_last_bytecode_index(struct Context *ctx)
{
    return ctx->compiling_chunk->bytecode.len - 1;
}
u8 *get_bytecode_byte(struct Context *ctx, u32 index)
{
    if (index > ctx->compiling_chunk->bytecode.len)
        panic("out of bounds byte index");

    return &ctx->compiling_chunk->bytecode.ptr[index];
}

u32 create_constant(struct Context *ctx, enum ObjType type, u32 size)
{
    struct ConstantList *constants = &ctx->compiling_chunk->constants;
    if (constants->len + size >= constants->cap) {
        u32 new_cap = (constants->cap == 0) ? 4 : constants->cap * 2;
        u64 *new_ptr = realloc_mem(constants->ptr, new_cap * sizeof(u64));
        constants->ptr = new_ptr;
        constants->cap = new_cap;
    }
    u32 index = constants->len;
    constants->len += size;
    return index;
}
u64 *get_constant(struct Context *ctx, u32 index)
{
    struct ConstantList *constants = &ctx->compiling_chunk->constants;
    if (index > constants->len)
        return null;

    return &constants->ptr[index];
}
u16 create_closure_info(struct Context *ctx, struct ClosureInfo info)
{
    struct ClosureInfoList *closures = &ctx->compiling_chunk->closures;

    if (closures->len == closures->cap) {
        u32 new_cap = (closures->cap == 0) ? 4 : closures->cap * 2;
        struct ClosureInfo *new_ptr = realloc_mem(closures->ptr, new_cap * sizeof(struct ClosureInfoList));
        closures->ptr = new_ptr;
        closures->cap = new_cap;
    }
    u16 index = closures->len;
    closures->ptr[closures->len++] = info;
    return index;
}

