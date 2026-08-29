#include "codegen.h"
#include "expr.h"
#include "global_pass.h"
#include "../builtins.h"
#include "global_resolution.h"
#include "remapping.h"
#include <string.h>

struct TopLevelDeclInfo {
    u32 constant_index;
    u16 variant_index;
    u8 arg_count;
};

static struct TopLevelDeclInfo compile_top_level_decl(struct Context *ctx, struct DeclarationNode *node)
{
    static u16 variant_count = 0;
    u32 function_start_index = get_last_bytecode_index(ctx) + 1;

    u16 type_info_index = get_module_globals(ctx->globals, ctx->module_index)->type_info_index;

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
        u16 variant = -1;
        if (lambda->body->kind == AST_CONSTRUCTOR) {
            variant = variant_count++;
            emit_byte(ctx, OP_CREATE_OBJECT);
            emit_u16(ctx, type_info_index);
            emit_u16(ctx, variant);
            emit_u16(ctx, bindings);
            if (type_info_index == 0)
                panic("unreachable: type should have been compiled");
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

        if (const_closure != null) {
            const_closure->info = (struct ClosureInfo*)(u64)closure_info;
        }

        set_global_decl_info(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            (struct GlobalInfo){
                .constant_index = constant_index,
                .variant_index = variant,
                .arg_count = bindings,
            }
        );

        return (struct TopLevelDeclInfo){ constant_index, variant, bindings };
    } else {
        u16 variant = -1;
        if (node->body->kind == AST_CONSTRUCTOR) {
            variant = variant_count++;
            emit_byte(ctx, OP_CREATE_OBJECT);
            emit_u16(ctx, type_info_index);
            emit_u16(ctx, variant);
            emit_u16(ctx, 0);
            if (type_info_index == 0) {
                panic("unreachable: type should have been compiled");
            }
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

        if (const_thunk != null) {
            const_thunk->evaluated = null;
            const_thunk->info = (struct ClosureInfo*)(u64)closure_info;
        }

        set_global_decl_info(
            get_module_globals(ctx->globals, ctx->module_index),
            node,
            (struct GlobalInfo){
                .constant_index = constant_index,
                .variant_index = variant,
                .arg_count = 0,
            }
        );

        return (struct TopLevelDeclInfo){ constant_index, variant, 0 };
    }
}

static bool try_remap_from_globals(struct Context *ctx, struct GlobalCtx *globals, struct RemappingWork remapping)
{
    struct GlobalInfo global_info = {};

    if (remapping.is_namespace) {
        struct GlobalResolutionResult result = resolve_global_path(
            globals,
            (struct GlobalSearch){
                .searching_for = remapping.namespace_access,
                .origin_module = remapping.searching_from_module,
            },
            &global_info
        );
        if (result.error != GLOBAL_RES_OK) {
            namespace_access_error(ctx, result);
            return false;
        }
        if (global_info.constant_index == -1 && ctx->compiling_chunk != null) {
            return false;
        }
    } else {
        enum GlobalResolutionError error = resolve_global(
            globals,
            remapping.searching_from_module,
            remapping.identifier->src_loc,
            &global_info
        );
        if (error != GLOBAL_RES_OK) {
            non_existent_ident_err(ctx, remapping.identifier->node.loc, null);
            return false;
        }
        if (global_info.constant_index == -1 && ctx->compiling_chunk != null) {
            return false;
        }
    }

    u8 *constant_index_bytes = get_bytecode_byte(ctx, remapping.bytecode_index);

    if (remapping.is_pattern_constructor) {
        if (global_info.variant_index == (u16)-1)
            return false;
        if (ctx->compiling_chunk == null)
            return true;

        if (global_info.arg_count != remapping.arg_count) {
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
            constant_index_bytes[i] = ((u8*)&global_info.variant_index)[i];
        }
    } else {
        if (constant_index_bytes != null) {
            for (u32 i = 0; i < sizeof(u32); i++) {
                constant_index_bytes[i] = ((u8*)&global_info.constant_index)[i];
            }
        }
    }
    return true;
}

static void clear_remapping_queue(struct Context *ctx)
{
    while (ctx->remapping_queue->len > 0) {
        struct RemappingWork remapping = dequeue_remapping_work(ctx->remapping_queue);

        if (try_remap_from_globals(ctx, ctx->globals, remapping)) {
            continue;
        }
        if (ctx->errors->len > 0) {
            break;
        }

        struct DeclarationNode *decl_node = null;

        u16 module_index = 0;
        struct GlobalSearch search = {
            .searching_for = remapping.is_namespace
                                ? remapping.namespace_access
                                : (struct NamespaceAccessNode*)remapping.identifier,
            .origin_module = remapping.searching_from_module,
        };
        struct GlobalResolutionResult result = find_global_decl(
            ctx->globals,
            search,
            &decl_node,
            &module_index
        );
        if (result.error != GLOBAL_RES_OK) {
            namespace_access_error(ctx, result);
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
                enqueue_remapping_work(ctx->remapping_queue, remapping);
                continue;
            }
        }

        ctx->module_index = module_index;
        {
            struct Module *mod = get_module(ctx->globals->modules, ctx->module_index);
            if (is_type_module(mod)) {
                struct ModuleGlobals *globals = get_module_globals(ctx->globals, ctx->module_index);
                if (globals->type_info_index == 0) {
                    char *name = alloc_mem(mod->name_len);
                    memcpy(name, mod->name, mod->name_len);
                    globals->type_info_index = create_type_info(ctx, (struct TypeInfo){
                        .name = name,
                        .name_len = mod->name_len,
                    });
                }
            }
        }
        struct TopLevelDeclInfo decl_info = compile_top_level_decl(ctx, decl_node);

        u8 *bytecode_ptr = get_bytecode_byte(ctx, remapping.bytecode_index);

        if (remapping.is_pattern_constructor) {
            if (decl_node->body->kind != AST_CONSTRUCTOR) {
                if (decl_node->body->kind != AST_LAMBDA
                    || ((struct LambdaNode*)decl_node->body)->body->kind != AST_CONSTRUCTOR
                ) {
                    invalid_pattern_err(
                        ctx,
                        remapping.loc,
                        "not a constructor"
                    );
                }
            }
            if (ctx->compiling_chunk == null)
                continue;

            if (decl_info.arg_count != remapping.arg_count) {
                invalid_pattern_err(
                    ctx,
                    remapping.loc,
                    "wrong arg count"
                );
            }

            for (u32 i = 0; i < sizeof(u16); i++) {
                bytecode_ptr[i] = ((u8*)&decl_info.variant_index)[i];
            }
        } else {
            if (bytecode_ptr != null) {
                for (u32 i = 0; i < sizeof(u32); i++) {
                    bytecode_ptr[i] = ((u8*)&decl_info.constant_index)[i];
                }
            }
        }
    }


}

struct CodegenErrorList generate_code(struct Compiler *compiler, struct ModuleCtx *modules)
{
    const char *main_ident = ident_table_get(&compiler->identifiers, "main", 4);

    struct CodegenErrorList errors = {};

    struct GlobalCtx globals = run_global_pass(compiler, modules, &errors);

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

        struct Location loc = main_decl->body->loc;

        static struct IdentifierNode STD_IDENT, RUN_IDENT;
        static struct NamespaceAccessNode NAMESPACE_NODE;
        static struct ApplicationNode APPLICATION_NODE;
        STD_IDENT = (struct IdentifierNode){
            .node = { AST_IDENTIFIER, loc },
            .src_loc = ident_table_get(ctx.identifier_table, "std", 3),
            .len = 3,
        };
        RUN_IDENT = (struct IdentifierNode){
            .node = { AST_IDENTIFIER, loc },
            .src_loc = ident_table_get(ctx.identifier_table, "run", 3),
            .len = 3,
        };
        NAMESPACE_NODE = (struct NamespaceAccessNode){
            .node = { AST_NAMESPACE_ACCESS, loc },
            .ident = &STD_IDENT,
            .rhs = AS_NODE(&RUN_IDENT),
        };
        APPLICATION_NODE = (struct ApplicationNode){
            .node = { AST_APPLICATION, loc },
            .function = AS_NODE(&NAMESPACE_NODE),
            .argument = main_decl->body,
        };

        compile_expr(&ctx, AS_NODE(&APPLICATION_NODE));

        emit_2_bytes(&ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
        emit_byte(&ctx, OP_U64_ADD);
        emit_u64(&ctx, 12);
        emit_byte(&ctx, OP_SWAP);
        emit_2_bytes(&ctx, OP_JUMP_GLOBALS, GLOBAL_FUNC_FORCE);
    }
    emit_byte(&ctx, OP_END);

    clear_remapping_queue(&ctx);

    ctx.compiling_chunk = null;
    for (u16 mod = 0; mod < globals.len; mod++) {
        struct ModuleGlobals *module = get_module_globals(&globals, mod);
        ctx.module_index = mod;
        
        for (u16 i = 0; i < module->len; i++) {
            struct Global *global = &module->globals[i];

            if (global->constant_index != (u16)-1) {
                continue;
            }

            compile_top_level_decl(&ctx, global->node);
        }
    }
    clear_remapping_queue(&ctx);

    free_remapping_queue(&remapping_queue);
    free_global_ctx(&globals);

    return errors;
}

