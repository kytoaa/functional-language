#include "codegen.h"
#include "expr.h"
#include "global_pass.h"
#include "../builtins.h"
#include "global_resolution.h"
#include "remapping.h"

static u32 compile_top_level_decl(struct Context *ctx, struct DeclarationNode *node)
{
    printf("compiling %.*s in %d\n", node->name_len, node->name, ctx->module_index);
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

        u32 constant_index = create_constant(ctx->compiling_chunk, OBJ_CLOSURE, sizeof(struct Closure));
        struct Closure *const_closure = (struct Closure*)get_constant(ctx, constant_index);

        u16 closure_info = create_closure_info(ctx, (struct ClosureInfo){
            .arity = bindings,
            .address = function_start_index,
            .capture_count = 0,
        });

        const_closure->info = (struct ClosureInfo*)(u64)closure_info;

        set_global_decl_const_index(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            constant_index
        );

        return constant_index;
    } else {
        compile_expr(ctx, node->body);
        emit_byte(ctx, OP_SWAP);
        emit_byte(ctx, OP_JUMP);

        u32 constant_index = create_constant(ctx->compiling_chunk, OBJ_THUNK, sizeof(struct Thunk));
        struct Thunk *const_thunk = (struct Thunk*)get_constant(ctx, constant_index);

        u16 closure_info = create_closure_info(ctx, (struct ClosureInfo){
            .arity = 0,
            .address = function_start_index,
            .capture_count = 0,
        });

        const_thunk->evaluated = null;
        const_thunk->info = (struct ClosureInfo*)(u64)closure_info;

        set_global_decl_const_index(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            constant_index
        );

        return constant_index;
    }
}

static bool try_remap_from_globals(struct Context *ctx, struct GlobalCtx *globals, struct RemappingWork remapping)
{
    u32 constant_index = 0;

    if (remapping.is_namespace) {
        struct GlobalResolutionResult result = resolve_global_path(
            globals,
            (struct GlobalSearch){
                .searching_for = remapping.namespace_access,
                .origin_module = remapping.searching_from_module,
            },
            &constant_index
        );
        if (result.error != GLOBAL_RES_OK) {
            namespace_access_error(ctx, result);
            return false;
        }
        if (constant_index == -1) {
            return false;
        }
    } else {
        enum GlobalResolutionError error = resolve_global(globals, remapping.searching_from_module, remapping.identifier->src_loc, &constant_index);
        if (error != GLOBAL_RES_OK) {
            non_existent_ident_err(ctx, remapping.identifier->node.loc, null);
            return false;
        }
        if (constant_index == -1) {
            return false;
        }
    }

    u8 *constant_index_bytes = get_bytecode_byte(ctx, remapping.bytecode_index);

    for (u32 i = 0; i < 4; i++) {
        constant_index_bytes[i] = ((u8*)&constant_index)[i];
    }
    return true;
}

struct CodegenErrorList generate_code(struct Compiler *compiler, struct ModuleCtx *modules)
{
    const char *main_ident = ident_table_get(&compiler->identifiers, "main", 4);

    struct GlobalCtx globals = run_global_pass(compiler, modules);

    struct CodegenErrorList errors = {};

    struct RemappingQueue remapping_queue = {};

    struct Context ctx = {
        .parent = null,
        .identifier_table = &compiler->identifiers,
        .compiling_chunk = &compiler->chunk,
        .errors = &errors,
        .globals = &globals,
        .remapping_queue = &remapping_queue,
        .module_index = 0,
    };

    struct DeclarationNode *main_decl = (struct DeclarationNode*)get_compiled_file(compiler, 0)->ast.declarations;
    while (main_decl != null) {
        if (main_decl->name == main_ident) {
            break;
        }
        main_decl = main_decl->next_declaration;
    }

    if (main_ident != null && main_decl != null) {
        ctx.module_index = 0;

        compile_expr(&ctx, main_decl->body);

        emit_2_bytes(&ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
        emit_byte(&ctx, OP_U64_ADD);
        emit_u64(&ctx, 12);
        emit_byte(&ctx, OP_SWAP);
        emit_2_bytes(&ctx, OP_JUMP_GLOBALS, GLOBAL_FUNC_FORCE);
    }
    emit_byte(&ctx, OP_END);

    while (remapping_queue.len > 0) {
        struct RemappingWork remapping = dequeue_remapping_work(&remapping_queue);

        if (try_remap_from_globals(&ctx, &globals, remapping)) {
            continue;
        }
        if (errors.len > 0) {
            break;
        }

        struct DeclarationNode *decl_node = null;

        u16 module_index = 0;
        struct GlobalResolutionResult result = find_global_decl(
            &globals,
            remapping.is_namespace ? AS_NODE(remapping.namespace_access) : AS_NODE(remapping.identifier),
            remapping.searching_from_module,
            &decl_node,
            &module_index
        );
        if (result.error != GLOBAL_RES_OK) {
            namespace_access_error(&ctx, result);
        }

        ctx.module_index = module_index;
        u32 constant_index = compile_top_level_decl(&ctx, decl_node);

        u8 *bytecode_ptr = get_bytecode_byte(&ctx, remapping.bytecode_index);
        for (u32 i = 0; i < 4; i++) {
            bytecode_ptr[i] = ((u8*)&constant_index)[i];
        }
    }

    free_remapping_queue(&remapping_queue);

    free_global_ctx(&globals);

    return errors;
}

