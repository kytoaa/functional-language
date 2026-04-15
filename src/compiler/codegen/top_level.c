#include "codegen.h"
#include "../../vm/extern_functions.h"

void compile_top_level(struct Context *ctx, struct DeclarationNode *first_decl)
{
    const char *main_ident = ident_table_get(ctx->identifier_table, "main", 4);

    struct DeclarationNode *decl = first_decl;
    while (decl != null) {
        compile_declaration(ctx, decl);
        if (decl->name == main_ident) {
            if (decl->body->kind == AST_LAMBDA) {
                panic("main cannot have any arguments");
            }
        }
        decl = decl->next_declaration;
    }

    if (main_ident != null) {
        struct IdentSearchResult main_function = {};
        if (get_ident_info(ctx, main_ident, &main_function)) {
            emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
            emit_byte(ctx, OP_U64_ADD);
            emit_u64(ctx, 1 + 8 + 1 + 2 + 1);
            emit_byte(ctx, OP_READ_BINDING);
            emit_u16(ctx, main_function.offset);
            emit_byte(ctx, OP_EVAL);
            emit_byte(ctx, OP_CALL_EXTERN);
            emit_u64(ctx, (u64)print_stack_val);
        }
    }
    emit_byte(ctx, OP_END);
}

