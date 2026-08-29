#include "global_pass.h"
#include "codegen.h"
#include "global_resolution.h"
#include "../module_resolution.h"
#include "../file_compilation.h"

static enum GlobalDeclError global_decl(
    struct ModuleGlobals *globals,
    struct Chunk *chunk,
    struct DeclarationNode *node,
    bool is_public,
    bool is_type_module,
    struct Location *out_err_loc
) {
    return declare_global_decl(globals, node, -1, -1, is_public, is_type_module, out_err_loc);
}

static void handle_global_decl_result(
    struct CodegenErrorList *errors,
    enum GlobalDeclError err,
    struct Location decl_loc,
    struct Location module_loc,
    struct Location err_loc,
    u16 file_index
) {
    if (err == GLOBAL_DECL_ERROR_REDECLARED) {
        push_codegen_err(
            errors,
            (struct CodegenError){
                .additional_msg = null,
                .type = CODEGEN_ERR_REDECLARED_GLOBAL,
                .file_index = file_index,
                .error = {
                    .redeclared_global = {
                        .loc = decl_loc,
                        .prev_decl_loc = err_loc,
                    },
                },
            }
        );
    } else if (err == GLOBAL_DECL_ERROR_MOD_NOT_TYPE) {
        push_codegen_err(
            errors,
            (struct CodegenError){
                .additional_msg = null,
                .type = CODEGEN_ERR_MOD_NOT_TYPE,
                .file_index = file_index,
                .error = {
                    .mod_not_type = {
                        .loc = decl_loc,
                        .mod_loc = module_loc,
                    },
                },
            }
        );
    }
}

static void run_global_pass_on(
    struct Compiler *compiler,
    struct GlobalCtx *global_ctx,
    struct Chunk *chunk,
    u16 module_index,
    struct CodegenErrorList *errors
) {
    struct Module *module = get_module(global_ctx->modules, module_index);
    struct ModuleGlobals *module_globals = get_module_globals(global_ctx, module_index);
    module_globals->module = module_index;
    u16 file_index = compiled_file_index(module);
    bool type_module = is_type_module(module);

    struct Location err_loc = {};

    if (is_file_module(module)) {
        struct AstTopLevel *ast = &get_compiled_file(compiler, compiled_file_index(module))->ast;

        struct DeclarationNode *decl = (struct DeclarationNode*)ast->declarations;
        while (decl != null) {
            enum GlobalDeclError decl_error = global_decl(
                module_globals,
                chunk,
                decl,
                false,
                type_module,
                &err_loc
            );
            handle_global_decl_result(
                errors,
                decl_error,
                decl->node.loc,
                module->loc,
                err_loc,
                file_index
            );
            decl = decl->next_declaration;
        }
    }
    for (u32 i = 0; i < module->items.len; i++) {
        struct ModuleItem *item = &module->items.ptr[i];
        if (item->is_submodule)
            continue;

        enum GlobalDeclError decl_error = global_decl(
            module_globals,
            chunk,
            item->decl_node,
            true,
            type_module,
            &err_loc
        );
        handle_global_decl_result(
            errors,
            decl_error,
            item->decl_node->node.loc,
            module->loc,
            err_loc,
            file_index
        );
    }
}

struct GlobalCtx run_global_pass(
    struct Compiler *compiler,
    struct ModuleCtx *modules,
    struct CodegenErrorList *errors
) {
    struct GlobalCtx global_ctx = {};
    init_global_ctx(&global_ctx, modules);

    for (u16 i = 0; i < modules->modules.count; i++) {
        run_global_pass_on(compiler, &global_ctx, &compiler->chunk, i, errors);
    }

    return global_ctx;
}

