#include "attributes.h"
#include "codegen.h"
#include <string.h>

static bool try_compile_std_attribute(struct Context *ctx, struct AttributeNode *node);
static void compile_builtin(struct Context *ctx, struct AstNode *body);

void compile_attribute(struct Context *ctx, struct AttributeNode *node)
{
    struct Module *current_module = get_module(ctx->globals->modules, ctx->module_index);
    while (current_module->parent_index != (u16)-1)
        current_module = get_module(ctx->globals->modules, current_module->parent_index);

    if (current_module->name == ctx->globals->modules->std_ident
        && current_module->parent_index == (u16)-1)
    {
        if (try_compile_std_attribute(ctx, node))
            return;
    }
}

static bool try_compile_std_attribute(struct Context *ctx, struct AttributeNode *node)
{
    const char *builtin_function_ident = ident_table_get(ctx->identifier_table, "std_builtin", 11);

    if (node->ident->src_loc == builtin_function_ident) {
        compile_builtin(ctx, node->body);
        return true;
    }

    return false;
}

static void compile_builtin(struct Context *ctx, struct AstNode *body)
{
    if (body == null)
        return;

    const char *write_ident = ident_table_get(ctx->identifier_table, "write", 5);
    const char *stdin_ident = ident_table_get(ctx->identifier_table, "stdin", 5);
    const char *stdout_ident = ident_table_get(ctx->identifier_table, "stdout", 6);
    const char *stderr_ident = ident_table_get(ctx->identifier_table, "stderr", 6);

    const char *read_file_contents_ident = ident_table_get(ctx->identifier_table, "read_file_contents", 18);
    const char *read_file_line_ident = ident_table_get(ctx->identifier_table, "read_file_line", 14);

    const char *type_of_ident = ident_table_get(ctx->identifier_table, "type_of", 7);

    const char *slice_empty_ident = ident_table_get(ctx->identifier_table, "slice_empty", 11);
    const char *slice_len_ident = ident_table_get(ctx->identifier_table, "slice_len", 9);
    const char *slice_index_ident = ident_table_get(ctx->identifier_table, "slice_index", 11);
    const char *slice_drop_ident = ident_table_get(ctx->identifier_table, "slice_drop", 10);
    const char *slice_take_ident = ident_table_get(ctx->identifier_table, "slice_take", 10);
    const char *slice_join_ident = ident_table_get(ctx->identifier_table, "slice_join", 10);
    const char *slice_cons_ident = ident_table_get(ctx->identifier_table, "slice_cons", 10);
    const char *slice_push_ident = ident_table_get(ctx->identifier_table, "slice_push", 10);

    switch (body->kind) {
        case AST_IDENTIFIER:{
            struct IdentifierNode *node = (struct IdentifierNode*)body;
            if (node->src_loc == write_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_WRITE);
                emit_byte(ctx, OP_PUSH_CONST);
                emit_u32(ctx, 0);
            } else if (node->src_loc == stdin_ident) {
                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_STDIN);
            } else if (node->src_loc == stdout_ident) {
                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_STDOUT);
            } else if (node->src_loc == stderr_ident) {
                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_STDERR);
            } else if (node->src_loc == read_file_contents_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_READ_CONTENTS);
            } else if (node->src_loc == read_file_line_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_READ_LINE);
            } else if (node->src_loc == type_of_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_TYPE_OF);
            } else if (node->src_loc == slice_empty_ident) {
                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_EMPTY);
            } else if (node->src_loc == slice_len_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_LEN);
            } else if (node->src_loc == slice_index_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_READ_SLICE_INDEX);
            } else if (node->src_loc == slice_drop_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_DROP);
            } else if (node->src_loc == slice_take_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_TAKE);
            } else if (node->src_loc == slice_join_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_JOIN);
            } else if (node->src_loc == slice_cons_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_CONS);
            } else if (node->src_loc == slice_push_ident) {
                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 0);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_READ_BINDING);
                emit_u16(ctx, 1);
                emit_byte(ctx, OP_EVAL);

                emit_byte(ctx, OP_CALL_EXTERN);
                emit_byte(ctx, VM_EXTERN_FUNC_SLICE_PUSH);
            } else {
                panic("not a builtin function");
            }
            break;
        }
        default:
            panic("not a builtin");
    }
}

