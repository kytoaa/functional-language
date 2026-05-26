#include "codegen.h"
#include "expr.h"
#include "global_pass.h"
#include "../builtins.h"
#include "global_resolution.h"
#include "remapping.h"

struct TopLevelDeclInfo {
    u32 constant_index;
    u16 closure_index;
};

static struct TopLevelDeclInfo compile_top_level_decl(struct Context *ctx, struct DeclarationNode *node)
{
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
        u32 constructor_variant = 0;
        if (lambda->body->kind == AST_CONSTRUCTOR) {
            emit_byte(ctx, OP_CREATE_OBJECT);
            emit_u16(ctx, 0);
            constructor_variant = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, 0);
            emit_u16(ctx, bindings);
        } else {
            compile_expr(ctx, lambda->body);
        }

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
        if (constructor_variant != 0) {
            u8 *constructor_variant_index = get_bytecode_byte(ctx, constructor_variant);
            for (u8 i = 0; i < sizeof(u16); i++) {
                constructor_variant_index[i] = ((u8*)&closure_info)[i];
            }
        }

        const_closure->info = (struct ClosureInfo*)(u64)closure_info;

        set_global_decl_const_index(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            constant_index,
            closure_info
        );

        return (struct TopLevelDeclInfo){ constant_index, closure_info };
    } else {
        u32 constructor_variant = 0;
        if (node->body->kind == AST_CONSTRUCTOR) {
            emit_byte(ctx, OP_CREATE_OBJECT);
            emit_u16(ctx, 0);
            constructor_variant = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, 0);
            emit_u16(ctx, 0);
        } else {
            compile_expr(ctx, node->body);
        }
        emit_byte(ctx, OP_SWAP);
        emit_byte(ctx, OP_JUMP);

        u32 constant_index = create_constant(ctx->compiling_chunk, OBJ_THUNK, sizeof(struct Thunk));
        struct Thunk *const_thunk = (struct Thunk*)get_constant(ctx, constant_index);

        u16 closure_info = create_closure_info(ctx, (struct ClosureInfo){
            .arity = 0,
            .address = function_start_index,
            .capture_count = 0,
        });
        if (constructor_variant != 0) {
            u8 *constructor_variant_index = get_bytecode_byte(ctx, constructor_variant);
            for (u8 i = 0; i < sizeof(u16); i++) {
                constructor_variant_index[i] = ((u8*)&closure_info)[i];
            }
        }

        const_thunk->evaluated = null;
        const_thunk->info = (struct ClosureInfo*)(u64)closure_info;

        set_global_decl_const_index(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            constant_index,
            closure_info
        );

        return (struct TopLevelDeclInfo){ constant_index, closure_info };
    }
}

static bool try_remap_from_globals(struct Context *ctx, struct GlobalCtx *globals, struct RemappingWork remapping)
{
    u32 constant_index = 0;
    u16 closure_info_index = 0;

    if (remapping.is_namespace) {
        struct GlobalResolutionResult result = resolve_global_path(
            globals,
            (struct GlobalSearch){
                .searching_for = remapping.namespace_access,
                .origin_module = remapping.searching_from_module,
            },
            &constant_index,
            &closure_info_index
        );
        if (result.error != GLOBAL_RES_OK) {
            namespace_access_error(ctx, result);
            return false;
        }
        if (constant_index == -1) {
            return false;
        }
    } else {
        enum GlobalResolutionError error = resolve_global(
            globals,
            remapping.searching_from_module,
            remapping.identifier->src_loc,
            &constant_index,
            &closure_info_index
        );
        if (error != GLOBAL_RES_OK) {
            non_existent_ident_err(ctx, remapping.identifier->node.loc, null);
            return false;
        }
        if (constant_index == -1) {
            return false;
        }
    }

    u8 *constant_index_bytes = get_bytecode_byte(ctx, remapping.bytecode_index);

    if (remapping.is_pattern_constructor) {
        if (closure_info_index == (u16)-1) {
            return false;
        }
        u16 expected_args = ctx->compiling_chunk->closures.ptr[closure_info_index].arity;

        if (expected_args != remapping.arg_count) {
            invalid_pattern_err(
                ctx,
                remapping.is_namespace
                    ? remapping.namespace_access->node.loc
                    : remapping.identifier->node.loc,
                "wrong arg count"
            );
            return true;
        }

        for (u32 i = 0; i < sizeof(u16); i++) {
            constant_index_bytes[i] = ((u8*)&closure_info_index)[i];
        }
    } else {
        for (u32 i = 0; i < sizeof(u32); i++) {
            constant_index_bytes[i] = ((u8*)&constant_index)[i];
        }
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

        if (decl_node->body->kind == AST_CONSTRUCTOR) {
            struct ConstructorNode *constructor = (struct ConstructorNode*)decl_node->body;

            if (constructor->body != null) {
                remapping.searching_from_module = module_index;
                if (constructor->body->kind == AST_NAMESPACE_ACCESS) {
                    remapping.is_namespace = true;
                    remapping.namespace_access = (struct NamespaceAccessNode*)constructor->body;
                } else {
                    remapping.is_namespace = false;
                    remapping.identifier = (struct IdentifierNode*)constructor->body;
                }
                enqueue_remapping_work(&remapping_queue, remapping);
                continue;
            }
        }

        ctx.module_index = module_index;
        struct TopLevelDeclInfo decl_info = compile_top_level_decl(&ctx, decl_node);

        u8 *bytecode_ptr = get_bytecode_byte(&ctx, remapping.bytecode_index);

        if (remapping.is_pattern_constructor) {
            if (decl_node->body->kind != AST_CONSTRUCTOR) {
                if (decl_node->body->kind != AST_LAMBDA
                    || ((struct LambdaNode*)decl_node->body)->body->kind != AST_CONSTRUCTOR
                ) {
                    invalid_pattern_err(
                        &ctx,
                        remapping.loc,
                        "not a constructor"
                    );
                }
            }
            u16 expected_args = ctx.compiling_chunk->closures.ptr[decl_info.closure_index].arity;

            if (expected_args != remapping.arg_count) {
                invalid_pattern_err(
                    &ctx,
                    remapping.loc,
                    "wrong arg count"
                );
            }

            for (u32 i = 0; i < sizeof(u16); i++) {
                bytecode_ptr[i] = ((u8*)&decl_info.closure_index)[i];
            }
        } else {
            for (u32 i = 0; i < sizeof(u32); i++) {
                bytecode_ptr[i] = ((u8*)&decl_info.constant_index)[i];
            }
        }
    }

    free_remapping_queue(&remapping_queue);

    free_global_ctx(&globals);

    return errors;
}

