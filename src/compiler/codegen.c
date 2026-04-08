#include "../prelude.h"
#include "../parsing/ast.h"
#include "../parsing/nodes.h"
#include "../parsing/ident_table.h"
#include "../bytecode.h"

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

static u8 get_ident_offset(struct Context *ctx, const char *ident) 
{
    for (u8 i = 0; i < ctx->ident_stack_len; i++) {
        if (ctx->ident_stack[IDENT_STACK_SIZE - (i + 1)] == ident) {
            return i;
        }
    }
}

static void compile_expr(struct Context *ctx, struct AstNode *node);

static void compile_global_declaration(struct Context *ctx, struct DeclarationNode *node)
{
    struct FunctionBindingNode *binding = node->bindings;
    while (binding != null) {
        declare_ident(ctx, binding->src_loc);
        binding = binding->next_binding;
    }
}

static void compile_expr(struct Context *ctx, struct AstNode *node)
{
    
}

