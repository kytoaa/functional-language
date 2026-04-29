#include "codegen.h"
#include "expr.h"
#include "../../vm/extern_functions.h"
#include "global_pass.h"
#include "global_resolution.h"

static void compile_top_level_decl(struct Context *ctx, struct DeclarationNode *node)
{
    u32 global_const_index = 0;
    resolve_global(ctx->globals, ctx->module_index, node->name, &global_const_index);

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

struct CodegenErrorList generate_code(struct Compiler *compiler, struct ModuleCtx *modules)
{
    const char *main_ident = ident_table_get(&compiler->identifiers, "main", 4);

    struct GlobalCtx globals = run_global_pass(compiler, modules);

    struct CodegenErrorList errors = {};

    struct Context ctx = {
        .parent = null,
        .identifier_table = &compiler->identifiers,
        .compiling_chunk = &compiler->chunk,
        .errors = &errors,
        .globals = &globals,
        .module_index = 0,
    };

    emit_byte(&ctx, OP_PUSH_U64);
    u32 main_jump_location = get_last_bytecode_index(&ctx) + 1;
    emit_u64(&ctx, 0);
    emit_byte(&ctx, OP_JUMP);

    struct DeclarationNode *main_decl = null;

    for (u16 module = 0; module < globals.len; module++) {
        ctx.module_index = module;
        struct ModuleGlobals *mod = get_module_globals(&globals, module);

        for (u32 i = 0; i < mod->len; i++) {
            struct Global *global = &mod->globals[i];
            if (module == 0 && global->node->name == main_ident) {
                main_decl = global->node;
                continue;
            }
            printf("global %.*s\n", global->node->name_len, global->node->name);
            compile_top_level_decl(&ctx, mod->globals[i].node);
        }
    }

    if (main_ident != null && main_decl != null) {
        ctx.module_index = 0;

        u64 main_start_addr = get_last_bytecode_index(&ctx) + 1;
        u8 *main_start_addr_bytes = (u8*)&main_start_addr;
        u8 *main_jump_location_bytes = get_bytecode_byte(&ctx, main_jump_location);
        for (u8 i = 0; i < 8; i++) {
            main_jump_location_bytes[i] = main_start_addr_bytes[i];
        }

        compile_expr(&ctx, main_decl->body);

        emit_byte(&ctx, OP_EVAL);
        emit_byte(&ctx, OP_CALL_EXTERN);
        emit_u64(&ctx, (u64)print_stack_val);
    }
    emit_byte(&ctx, OP_END);

    return errors;
}

