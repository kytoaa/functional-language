#include "global_resolution.h"
#include "../module_resolution.h"
#include "../file_compilation.h"

static void global_decl(struct ModuleGlobals *globals, struct Chunk *chunk, struct DeclarationNode *node, bool is_public)
{
    declare_global_decl(globals, node, -1, -1, is_public);
}

static void run_global_pass_on(
    struct Compiler *compiler,
    struct GlobalCtx *global_ctx,
    struct Chunk *chunk,
    u16 module_index
) {
    struct Module *module = get_module(global_ctx->modules, module_index);
    struct ModuleGlobals *module_globals = get_module_globals(global_ctx, module_index);
    module_globals->module = module_index;

    if (is_file_module(module)) {
        struct AstTopLevel *ast = &get_compiled_file(compiler, compiled_file_index(module))->ast;

        struct DeclarationNode *decl = (struct DeclarationNode*)ast->declarations;
        while (decl != null) {
            global_decl(module_globals, chunk, decl, false);
            decl = decl->next_declaration;
        }
    }
    for (u32 i = 0; i < module->items.len; i++) {
        struct ModuleItem *item = &module->items.ptr[i];
        if (item->is_submodule)
            continue;

        global_decl(module_globals, chunk, item->decl_node, true);
    }
}

struct GlobalCtx run_global_pass(struct Compiler *compiler, struct ModuleCtx *modules)
{
    struct GlobalCtx global_ctx = {};
    init_global_ctx(&global_ctx, modules);

    for (u16 i = 0; i < modules->modules.count; i++) {
        run_global_pass_on(compiler, &global_ctx, &compiler->chunk, i);
    }

    return global_ctx;
}

