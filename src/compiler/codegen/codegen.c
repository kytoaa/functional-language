#include "codegen.h"
#include "../../prelude.h"
#include "../../bytecode.h"
#include "../../object.h"
#include <string.h>

static void codegen_error(struct Context *ctx, struct CodegenError error);

void init_context(struct Context *ctx, struct Context *parent)
{
    ctx->parent = parent;
    ctx->identifier_table = parent->identifier_table;
    ctx->compiling_chunk = parent->compiling_chunk;

    ctx->errors = parent->errors;
    ctx->globals = parent->globals;
    ctx->remapping_queue = parent->remapping_queue;
    ctx->ident_stack_len = 0;
    ctx->capture_stack_len = 0;
    ctx->module_index = parent->module_index;
}
void end_context(struct Context *ctx)
{
    (void)ctx;
}

void declare_ident(struct Context *ctx, const char *ident)
{
    if (ctx->ident_stack_len == IDENT_STACK_SIZE) {
        panic("more than 256 identifiers");
    }
    ctx->ident_stack[ctx->ident_stack_len] = (struct Identifier){
        .ident = ident,
    };
    ctx->ident_stack_len += 1;
}
void drop_ident(struct Context *ctx, u32 count)
{
    if (ctx->ident_stack_len < count) {
        panic("unreachable: dropping too many identifiers");
    }
    ctx->ident_stack_len -= count;
}

bool get_ident_info(struct Context *ctx, const char *ident, struct IdentSearchResult *out)
{
    for (u16 i = 0; i < ctx->ident_stack_len; i++) {
        struct Identifier *searching_ident = &ctx->ident_stack[ctx->ident_stack_len - (i + 1)];
        if (searching_ident->ident == ident) {
            *out = (struct IdentSearchResult){
                .offset = i,
            };
            return true;
        }
    }
    return false;
}

static u8 add_capture(struct Context *ctx, u8 parent_index, bool is_local)
{
    for (u8 i = 0; i < ctx->capture_stack_len; i++) {
        if (ctx->capture_stack[i].parent_index == parent_index
            && ctx->capture_stack[i].is_local == is_local)
        {
            return i;
        }
    }
    if (ctx->capture_stack_len == IDENT_STACK_SIZE) {
        panic("unreachable: too many captures");
    }
    u8 index = ctx->capture_stack_len++;
    ctx->capture_stack[index] = (struct Capture){
        .parent_index = parent_index,
        .is_local = is_local
    };
    return index;
}

bool resolve_capture(struct Context *ctx, const char *ident, u8 *out)
{
    if (ctx->parent == null) {
        return false;
    }
    struct IdentSearchResult local = {};
    if (get_ident_info(ctx->parent, ident, &local)) {
        *out = add_capture(ctx, local.offset, true);
        return true;
    }
    u8 parent_index = 0;
    if (resolve_capture(ctx->parent, ident, &parent_index)) {
        *out = add_capture(ctx, parent_index, false);
        return true;
    }
    return false;
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
        u8 bytes[4];
        u32 val;
    } val = { .val = value };
    emit_byte(ctx, val.bytes[0]);
    emit_byte(ctx, val.bytes[1]);
    emit_byte(ctx, val.bytes[2]);
    emit_byte(ctx, val.bytes[3]);
}
void emit_u64(struct Context *ctx, u64 value)
{
    union {
        u8 bytes[8];
        u64 val;
    } val = { .val = value };
    emit_byte(ctx, val.bytes[0]);
    emit_byte(ctx, val.bytes[1]);
    emit_byte(ctx, val.bytes[2]);
    emit_byte(ctx, val.bytes[3]);
    emit_byte(ctx, val.bytes[4]);
    emit_byte(ctx, val.bytes[5]);
    emit_byte(ctx, val.bytes[6]);
    emit_byte(ctx, val.bytes[7]);
}
u32 get_last_bytecode_index(struct Context *ctx)
{
    return (ctx->compiling_chunk == null) ? -1 : ctx->compiling_chunk->bytecode.len - 1;
}
u8 *get_bytecode_byte(struct Context *ctx, u32 index)
{
    if (ctx->compiling_chunk == null)
        return null;

    if (index > ctx->compiling_chunk->bytecode.len)
        panic("unreachable: out of bounds byte index");

    return &ctx->compiling_chunk->bytecode.ptr[index];
}

u32 create_constant(struct Chunk *chunk, enum ObjType type, u32 size)
{
    if (chunk == null)
        return -1;

    size = (size + sizeof(u64) - 1) / sizeof(u64);
    struct ConstantList *constants = &chunk->constants;
    if (constants->len + size >= constants->cap) {
        u32 new_cap = (constants->cap == 0) ? size : constants->cap * 2;
        u64 *new_ptr = realloc_mem(constants->ptr, new_cap * sizeof(u64));
        constants->ptr = new_ptr;
        constants->cap = new_cap;
    }
    u32 index = constants->len;
    constants->len += size;

    struct Obj *obj = (struct Obj*)&constants->ptr[index];
    memset(obj, 0, size * sizeof(u64));

    obj->flags.is_static = true;
    obj->flags.gc_marked = false;
    obj->next = null;
    obj->type = type;

    if (type == OBJ_BOX) {
        obj_init_box((struct Box*)obj);
    }
    return index;
}
u64 *get_constant(struct Context *ctx, u32 index)
{
    if (ctx->compiling_chunk == null)
        return null;

    struct ConstantList *constants = &ctx->compiling_chunk->constants;
    if (index > constants->len)
        return null;

    return &constants->ptr[index];
}
u16 create_closure_info(struct Context *ctx, struct ClosureInfo info)
{
    if (ctx->compiling_chunk == null)
        return -1;

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

static void codegen_error(struct Context *ctx, struct CodegenError error)
{
    push_codegen_err(ctx->errors, error);
}
void push_codegen_err(struct CodegenErrorList *error_list, struct CodegenError error)
{
    if (error_list->cap == error_list->len) {
        u32 new_cap = (error_list->cap == 0) ? 1 : error_list->cap * 2;
        struct CodegenError *new_ptr = realloc_mem(error_list->ptr, new_cap * sizeof(struct CodegenError));
        error_list->ptr = new_ptr;
        error_list->cap = new_cap;
    }
    error_list->ptr[error_list->len++] = error;
}

void non_existent_ident_err(struct Context *ctx, struct Location loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_NON_EXISTENT_IDENT,
        .error = { .non_existent_identifier = { .loc = loc } },
    });
}
void redeclared_global_err(struct Context *ctx, struct Location loc, struct Location prev_decl_loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_REDECLARED_GLOBAL,
        .error = {
            .redeclared_global = {
                .loc = loc,
                .prev_decl_loc = prev_decl_loc,
            },
        },
    });
}
void main_args_err(struct Context *ctx, struct Location loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_MAIN_ARGS,
        .error = { .main_args = { .loc = loc } }
    });
}
void multiple_main_decl_err(struct Context *ctx, struct Location loc, struct Location prev_decl_loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_MULTIPLE_MAIN_DECL,
        .error = {
            .redeclared_global = {
                .loc = loc,
                .prev_decl_loc = prev_decl_loc,
            },
        },
    });
}
void used_underscore_err(struct Context *ctx, struct Location loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_USED_UNDERSCORE,
        .error = { .main_args = { .loc = loc } }
    });
}
void namespace_access_error(struct Context *ctx, struct GlobalResolutionResult error)
{
    const char *msg = null;
    switch (error.error) {
        case GLOBAL_RES_ERROR_PRIVATE:
            msg = "identifier is private";
            break;
        case GLOBAL_RES_ERROR_ROOT_SUPER:
            msg = "tried to get 'super' of the root module";
            break;
        case GLOBAL_RES_ERROR_RECURSION_LIMIT:
            msg = "reached recursion limit trying to evaluate path";
            break;
        default:
            break;
    }
    non_existent_ident_err(ctx, error.error_finding->node.loc, msg);
}
void invalid_pattern_err(struct Context *ctx, struct Location loc, const char *msg)
{
    codegen_error(ctx, (struct CodegenError){
        .additional_msg = msg,
        .type = CODEGEN_ERR_INVALID_PATTERN,
        .error = { .invalid_pattern = { .loc = loc } }
    });
}
struct CodegenError make_message_error(struct Location loc, const char *msg, u16 file_index)
{
    return (struct CodegenError){
        .file_index = file_index,
        .additional_msg = null,
        .type = CODEGEN_ERR_MSG,
        .error = { .with_message = { .msg = msg, .loc = loc } }
    };
}

void free_codegen_errors(struct CodegenErrorList *errors)
{
    free_mem(errors->ptr);
    errors->ptr = null;
}

