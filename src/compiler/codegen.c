#include "../prelude.h"
#include "../parsing/ast.h"
#include "../parsing/nodes.h"
#include "../parsing/ident_table.h"
#include "../bytecode.h"
#include "builtins.h"

#define IDENT_STACK_SIZE 256

struct Context {
    struct IdentifierTable *identifier_table;
    struct Chunk *compiling_chunk;
    const char *ident_stack[IDENT_STACK_SIZE];
    u32 ident_stack_len;
};

static void declare_ident(struct Context *ctx, const char *ident)
{
    if (ctx->ident_stack_len == IDENT_STACK_SIZE) {
        panic("more than 256 identifiers");
    }
    ctx->ident_stack[ctx->ident_stack_len] = ident;
    ctx->ident_stack_len += 1;
}

static void emit_byte(struct Context *ctx, u8 byte)
{
    chunk_write_byte(ctx->compiling_chunk, byte);
}
static void emit_2_bytes(struct Context *ctx, u8 byte, u8 arg)
{
    chunk_write_byte(ctx->compiling_chunk, byte);
    chunk_write_byte(ctx->compiling_chunk, arg);
}
static u8 *get_last_byte(struct Context *ctx)
{
    return ctx->compiling_chunk->bytecode.ptr + (ctx->compiling_chunk->bytecode.len - 1);
}

/// returns the ident's offset from the top of the stack
static u8 get_ident_offset(struct Context *ctx, const char *ident) 
{
    for (u8 i = 0; i < ctx->ident_stack_len; i++) {
        if (ctx->ident_stack[IDENT_STACK_SIZE - (i + 1)] == ident) {
            return i;
        }
    }
    return UINT8_MAX;
}

static void compile_expr(struct Context *ctx, struct AstNode *node);

static void compile_declaration(struct Context *ctx, struct DeclarationNode *node)
{
    struct FunctionBindingNode *binding = node->bindings;
    while (binding != null) {
        declare_ident(ctx, binding->src_loc);
        binding = binding->next_binding;
    }
}

static void compile_literal(struct Context *ctx, struct LiteralNode *node);
static void compile_bin_op(struct Context *ctx, struct BinOpNode *node);

static void compile_expr(struct Context *ctx, struct AstNode *node)
{
    // when function is evaluated it must be in whnf, therefore its argument count should be accessible
    switch (node->kind) {
        case AST_LITERAL:
            compile_literal(ctx, (struct LiteralNode*)node);
            break;
        case AST_BIN_OP:
            compile_bin_op(ctx, (struct BinOpNode*)node);
            break;
    }
}

static void compile_literal(struct Context *ctx, struct LiteralNode *node)
{
}

static void compile_bin_op(struct Context *ctx, struct BinOpNode *node)
{
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->l);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->r);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    emit_byte(ctx, OP_JUMP_GLOBALS);
    
    u8 op;
    switch (node->op) {
        case AST_BIN_OP_ADD:
            op = GLOBAL_FUNC_ADD;
            break;
        case AST_BIN_OP_SUB:
            op = GLOBAL_FUNC_SUB;
            break;
        case AST_BIN_OP_MUL:
            op = GLOBAL_FUNC_MUL;
            break;
        case AST_BIN_OP_DIV:
            op = GLOBAL_FUNC_DIV;
            break;
        default:
            panic("unreachable, invalid expression");
            break;
    }
    emit_byte(ctx, op);
}

static void compile_pattern_match(struct Context *ctx, struct AstNode *node)
{
    if (node->kind == AST_IDENTIFIER) {

    } else if (node->kind == AST_LITERAL) {
        emit_byte(ctx, OP_EVAL);
        // emit constant
        emit_byte(ctx, OP_EQUAL);
    }
}

static void compile_case_branch(struct Context *ctx, struct CasePatternNode *node)
{

}
