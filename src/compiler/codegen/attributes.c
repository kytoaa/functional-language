#include "attributes.h"

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
            } else {
                panic("not a builtin function");
            }
            break;
        }
        default:
            panic("not a builtin");
    }
}

