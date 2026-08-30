#include "bytecode.h"
#include "object.h"

#define BUILTIN_TYPE_COUNT 9

void init_chunk(struct Chunk *chunk)
{
    const u32 initial_size = sizeof(struct Box) * 3;
    u64 *constants_ptr = alloc_mem(initial_size);

    struct Box *unit_box = (struct Box*)constants_ptr;
    unit_box->obj = (struct Obj){
        .flags = { .is_whnf = true, .is_static = true },
        .next = null,
        .type = OBJ_BOX,
    };
    obj_init_box(unit_box);
    unit_box->val = UNIT_VAL();

    struct Box *true_box = unit_box + 1;
    true_box->obj = (struct Obj){
        .flags = { .is_whnf = true, .is_static = true },
        .next = null,
        .type = OBJ_BOX,
    };
    obj_init_box(true_box);
    true_box->val = BOOL_VAL(true);

    struct Box *false_box = unit_box + 2;
    false_box->obj = (struct Obj){
        .flags = { .is_whnf = true, .is_static = true },
        .next = null,
        .type = OBJ_BOX,
    };
    obj_init_box(false_box);
    false_box->val = BOOL_VAL(false);

    const u32 size = (initial_size + sizeof(u64) - 1) / sizeof(u64);

    const u32 type_info_size = sizeof(struct TypeInfo) * BUILTIN_TYPE_COUNT;
    struct TypeInfo *type_info_ptr = alloc_mem(type_info_size);

    type_info_ptr[0] = (struct TypeInfo){
        .name = "()",
        .name_len = 2,
    };
    type_info_ptr[1] = (struct TypeInfo){
        .name = "int",
        .name_len = 3,
    };
    type_info_ptr[2] = (struct TypeInfo){
        .name = "bool",
        .name_len = 4,
    };
    type_info_ptr[3] = (struct TypeInfo){
        .name = "char",
        .name_len = 4,
    };
    type_info_ptr[4] = (struct TypeInfo){
        .name = "Cons",
        .name_len = 4,
    };
    type_info_ptr[5] = (struct TypeInfo){
        .name = "function",
        .name_len = 8,
    };
    type_info_ptr[6] = (struct TypeInfo){
        .name = "File",
        .name_len = 4,
    };
    type_info_ptr[7] = (struct TypeInfo){
        .name = "Slice",
        .name_len = 5,
    };
    type_info_ptr[8] = (struct TypeInfo){
        .name = "type",
        .name_len = 4,
    };

    *chunk = (struct Chunk){
        .constants = {
            .ptr = constants_ptr,
            .cap = size,
            .len = size,
        },
        .types = {
            .ptr = type_info_ptr,
            .cap = 9,
            .len = 9,
        },
    };
}

void chunk_write_byte(struct Chunk *chunk, u8 byte)
{
    if (chunk == null)
        return;
    if (chunk->bytecode.len == chunk->bytecode.cap) {
        u32 new_cap = chunk->bytecode.cap == 0 ? 4 : chunk->bytecode.cap * 2;

        u8 *new_ptr = realloc_mem(chunk->bytecode.ptr, new_cap * sizeof(u8));
        chunk->bytecode.ptr = new_ptr;
        chunk->bytecode.cap = new_cap;
    }
    chunk->bytecode.ptr[chunk->bytecode.len++] = byte;
}

void free_chunk(struct Chunk *chunk)
{
    free_mem(chunk->bytecode.ptr);
    free_mem(chunk->constants.ptr);
    free_mem(chunk->closures.ptr);
    for (u32 i = BUILTIN_TYPE_COUNT; i < chunk->types.len; i++) {
        free_mem(chunk->types.ptr[i].name);
        chunk->types.ptr[i].name = null;
    }
    free_mem(chunk->types.ptr);
    *chunk = (struct Chunk){};
}

static const char *OP_NAMES[] = {
    [OP_NOOP] = "OP_NOOP",
    [OP_TRANSFER_STACK_REG] = "OP_TRANSFER_STACK_REG",
    [OP_COPY_STACK_REG] = "OP_COPY_STACK_REG",
    [OP_PUSH_REG_STACK] = "OP_PUSH_REG_STACK",
    [OP_SET_REG] = "OP_SET_REG",
    [OP_SWAP] = "OP_SWAP",
    [OP_COPY] = "OP_COPY",
    [OP_PUSH_U64] = "OP_PUSH_U64",
    [OP_POP_U64] = "OP_POP_U64",
    [OP_U64_ADD] = "OP_U64_ADD",
    [OP_READ_BINDING] = "OP_READ_BINDING",
    [OP_CREATE_BINDING] = "OP_CREATE_BINDING",
    [OP_REMOVE_BINDING] = "OP_REMOVE_BINDING",
    [OP_REMOVE_BINDINGS] = "OP_REMOVE_BINDINGS",
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
    [OP_OBJECT_READ] = "OP_OBJECT_READ",
    [OP_UPDATE_THUNK] = "OP_UPDATE_THUNK",
    [OP_PARTIAL_APPLY] = "OP_PARTIAL_APPLY",
    [OP_CREATE_CLOSURE] = "OP_CREATE_CLOSURE",
    [OP_WRITE_CLOSURE] = "OP_WRITE_CLOSURE",
    [OP_CREATE_OBJECT] = "OP_CREATE_OBJECT",
    [OP_CREATE_THUNK] = "OP_CREATE_THUNK",
    [OP_WRITE_THUNK] = "OP_WRITE_THUNK",
    [OP_CREATE_CONS] = "OP_CREATE_CONS",
    [OP_PUSH_CONST] = "OP_PUSH_CONST",
    [OP_TRUE] = "OP_TRUE",
    [OP_FALSE] = "OP_FALSE",
    [OP_EQUAL] = "OP_EQUAL",
    [OP_GREATER] = "OP_GREATER",
    [OP_LESS] = "OP_LESS",
    [OP_NOT] = "OP_NOT",
    [OP_ADD] = "OP_ADD",
    [OP_SUBTRACT] = "OP_SUBTRACT",
    [OP_MULTIPLY] = "OP_MULTIPLY",
    [OP_DIVIDE] = "OP_DIVIDE",
    [OP_NEGATE] = "OP_NEGATE",
    [OP_IS_CONS] = "OP_IS_CONS",
    [OP_IS_OBJ] = "OP_IS_OBJ",
    [OP_IS_INT] = "OP_IS_INT",
    [OP_IS_BOOL] = "OP_IS_BOOL",
    [OP_IS_CHAR] = "OP_IS_CHAR",
    [OP_IS_UNIT] = "OP_IS_UNIT",
    [OP_IS_VARIANT] = "OP_IS_VARIANT",
    [OP_HEAD] = "OP_HEAD",
    [OP_TAIL] = "OP_TAIL",
    [OP_CALL_EXTERN] = "OP_CALL_EXTERN",
    [OP_END] = "OP_END",
};

const char *bytecode_op_name(enum Bytecode byte)
{
    if (byte > OP_END)
        return null;

    return OP_NAMES[byte];
}
