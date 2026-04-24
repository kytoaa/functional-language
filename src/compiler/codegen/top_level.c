#include "codegen.h"
#include "expr.h"
#include "../../vm/extern_functions.h"

static void globals_pass(struct Context *ctx, struct DeclarationNode *node)
{
    const char *main_ident = ident_table_get(ctx->identifier_table, "main", 4);

    while (node != null) {
        if (node->name != main_ident) {
            u32 constant_index = 0;
            if (node->body->kind == AST_LAMBDA) {
                constant_index = create_constant(ctx, OBJ_CLOSURE, sizeof(struct Closure));
            } else {
                constant_index = create_constant(ctx, OBJ_THUNK, sizeof(struct Thunk));
            }

            declare_global(ctx, node->name, constant_index, node->node.loc);
        }

        node = node->next_declaration;
    }
}

static void compile_top_level_decl(struct Context *ctx, struct DeclarationNode *node)
{
    u32 global_const_index = 0;
    resolve_global(ctx, node->name, &global_const_index);

    u32 function_start_index = get_last_bytecode_index(ctx) + 1;

    if (node->body->kind == AST_LAMBDA) {
        struct LambdaNode *lambda = (struct LambdaNode*)node->body;
        struct FunctionBindingNode *binding = lambda->bindings;

        u32 bindings = 0;
        while (binding != null) {
            declare_ident(ctx, binding->src_loc);
            bindings += 1;
            binding = binding->next_binding;
            emit_byte(ctx, OP_CREATE_BINDING);
        }
        compile_expr(ctx, lambda->body);

        drop_ident(ctx, bindings);
        emit_2_bytes(ctx, OP_REMOVE_BINDINGS, bindings);
        emit_byte(ctx, OP_SWAP);
        emit_byte(ctx, OP_JUMP);

        struct Closure *const_closure = (struct Closure*)get_constant(ctx, global_const_index);

        u16 closure_info = create_closure_info(ctx, (struct ClosureInfo){
            .arity = bindings,
            .address = function_start_index,
            .capture_count = 0,
        });

        const_closure->info = (struct ClosureInfo*)(u64)closure_info;
    } else {
        compile_expr(ctx, node->body);
        emit_byte(ctx, OP_SWAP);
        emit_byte(ctx, OP_JUMP);

        struct Thunk *const_thunk = (struct Thunk*)get_constant(ctx, global_const_index);

        u16 closure_info = create_closure_info(ctx, (struct ClosureInfo){
            .arity = 0,
            .address = function_start_index,
            .capture_count = 0,
        });

        const_thunk->evaluated = null;
        const_thunk->info = (struct ClosureInfo*)(u64)closure_info;
    }
}

void compile_top_level(struct Context *ctx, struct AstTopLevel *top_level)
{
    const char *main_ident = ident_table_get(ctx->identifier_table, "main", 4);

    struct GlobalList globals = {};
    ctx->globals = &globals;

    emit_byte(ctx, OP_PUSH_U64);
    u32 main_jump_location = get_last_bytecode_index(ctx) + 1;
    emit_u64(ctx, 0);
    emit_byte(ctx, OP_JUMP);

    struct DeclarationNode *first_decl = (struct DeclarationNode*)top_level->declarations;

    globals_pass(ctx, first_decl);

    struct DeclarationNode *main_decl = null;

    struct DeclarationNode *decl = first_decl;
    while (decl != null) {
        if (decl->name == main_ident) {
            if (main_decl != null) {
                multiple_main_decl_err(ctx, decl->node.loc, main_decl->node.loc);
            }
            if (decl->body->kind == AST_LAMBDA) {
                main_args_err(ctx, decl->node.loc);
            }
            main_decl = decl;
        } else {
            compile_top_level_decl(ctx, decl);
        }
        decl = decl->next_declaration;
    }

    if (main_ident != null && main_decl != null) {
        u64 main_start_addr = get_last_bytecode_index(ctx) + 1;
        u8 *main_start_addr_bytes = (u8*)&main_start_addr;
        u8 *main_jump_location_bytes = get_bytecode_byte(ctx, main_jump_location);
        for (u8 i = 0; i < 8; i++) {
            main_jump_location_bytes[i] = main_start_addr_bytes[i];
        }

        compile_expr(ctx, main_decl->body);

        emit_byte(ctx, OP_EVAL);
        emit_byte(ctx, OP_CALL_EXTERN);
        emit_u64(ctx, (u64)print_stack_val);
    }
    emit_byte(ctx, OP_END);
}

