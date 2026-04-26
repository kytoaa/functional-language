#include "module_resolution.h"
#include "../parsing/ast.h"
#include "file_compilation.h"

static u16 create_module(struct ModuleCtx *ctx, const char *name, u16 parent)
{
    if (ctx->modules.count == ctx->modules.cap) {
        u32 new_cap = (ctx->modules.cap == 0) ? 2 : ctx->modules.cap * 2;
        struct Module *new_ptr = realloc_mem(ctx->modules.ptr, new_cap * sizeof(struct Module));
        ctx->modules.ptr = new_ptr;
        ctx->modules.cap = new_cap;
    }
    u16 index = ctx->modules.count;
    ctx->modules.ptr[ctx->modules.count++] = (struct Module){
        .name = name,
        .parent_index = parent,
    };
    return index;
}

static struct Module *get_module(struct ModuleCtx *ctx, u16 index)
{
    if (index >= ctx->modules.count)
        return null;

    return &ctx->modules.ptr[index];
}

static struct Module *find_module(struct ModuleCtx *ctx, const char *name, u16 parent)
{
    for (u16 i = 0; i < ctx->modules.count; i++) {
        struct Module *current = &ctx->modules.ptr[i];
        if (current->name == name && current->parent_index == parent) {
            return current;
        }
    }
    return null;
}

static void declare_item(struct Module *mod, struct ModuleItem item)
{
    if (mod->items.len == mod->items.cap) {
        u32 new_cap = (mod->items.cap == 0) ? 2 : mod->items.cap * 2;
        struct ModuleItem *new_ptr = realloc_mem(mod->items.ptr, new_cap * sizeof(struct ModuleItem));
        mod->items.ptr = new_ptr;
        mod->items.cap = new_cap;
    }
    mod->items.ptr[mod->items.len++] = item;
}

static void queue_resolution_work(struct ModuleCtx *ctx, struct ModuleResolutionWork work)
{
    if (ctx->worklist.len == ctx->worklist.cap) {
        u32 new_cap = (ctx->worklist.cap == 0) ? 2 : ctx->worklist.cap * 2;
        struct ModuleResolutionWork *new_ptr = realloc_mem(ctx->worklist.ptr, new_cap * sizeof(struct ModuleResolutionWork));
        ctx->worklist.ptr = new_ptr;
        ctx->worklist.cap = new_cap;
    }
    ctx->worklist.ptr[ctx->worklist.len++] = work;
}

static void queue_file_resolution(struct ModuleCtx *ctx, const char *name, u16 name_len, u16 parent_module)
{
    queue_resolution_work(ctx, (struct ModuleResolutionWork){
        .file = {
            .name = name,
            .name_len = name_len,
        },
        .work_kind = MODULE_WORK_FILE,
        .parent_module = parent_module,
    });
}
static void queue_node_resolution(struct ModuleCtx *ctx, struct ModuleDeclNode *node, u16 parent_module)
{
    queue_resolution_work(ctx, (struct ModuleResolutionWork){
        .ast_node = { .node = node },
        .work_kind = MODULE_WORK_AST_NODE,
        .parent_module = parent_module,
    });
}
static bool pop_resolution_queue(struct ModuleCtx *ctx, struct ModuleResolutionWork *out)
{
    if (ctx->worklist.len == 0)
        return false;

    *out = ctx->worklist.ptr[--ctx->worklist.len];
    return true;
}

static void declare_module_items(struct ModuleCtx *ctx, struct ModuleDeclNode *module, struct Module *mod)
{
    struct DeclarationNode *decl = module->declarations;
    while (decl != null) {
        declare_item(mod, (struct ModuleItem){
            .name = decl->name,
            .name_len = decl->name_len,
            .is_submodule = false,
        });

        decl = decl->next_declaration;
    }
    struct ModuleDeclNode *submodule = module->submodules;
    while (submodule != null) {
        declare_item(mod, (struct ModuleItem){
            .name = submodule->name->src_loc,
            .name_len = submodule->name->len,
            .is_submodule = true,
        });

        submodule = submodule->next_mod;
    }
}

static void resolve_node(struct ModuleCtx *ctx, struct ModuleDeclNode *node, u16 parent)
{
    u16 mod_index = create_module(ctx, node->name->src_loc, parent);
    struct Module *mod = get_module(ctx, mod_index);
    struct Module *parent_mod = get_module(ctx, parent);

    mod->compiled_file_index = parent_mod->compiled_file_index;

    declare_module_items(ctx, node, mod);
}

static void resolve_top_level(struct ModuleCtx *ctx, const struct AstTopLevel *ast, u16 file_index, const char *mod_name, u16 parent)
{
    u16 mod_index = create_module(ctx, mod_name, parent);
    struct Module *mod = get_module(ctx, mod_index);

    mod->compiled_file_index = file_index;

    struct ModuleDeclNode *submodule = (struct ModuleDeclNode*)ast->modules;
    while (submodule != null) {
        if (submodule->name == null) {
            declare_module_items(ctx, submodule, mod);
            continue;
        }

        if (!submodule->has_body) {
            queue_file_resolution(ctx, submodule->name->src_loc, submodule->name->len, mod_index);
        } else {
            queue_node_resolution(ctx, submodule, mod_index);
        }

        submodule = submodule->next_mod;
    }
}

struct ModuleCtx resolve_ast(struct Compiler *compiler, const struct CompiledFile *file)
{
    struct ModuleCtx ctx = {};
    resolve_top_level(&ctx, &file->ast, 0, "main", -1);

    struct ModuleResolutionWork work = {};
    while (pop_resolution_queue(&ctx, &work)) {
        switch (work.work_kind) {
            case MODULE_WORK_AST_NODE:{
                resolve_node(&ctx, work.ast_node.node, work.parent_module);
                break;
            }
            case MODULE_WORK_FILE:{
                struct CompiledFile *parent = get_compiled_file(compiler, get_module(&ctx, work.parent_module)->compiled_file_index);

                u32 file_index = compile_module(compiler, file, work.file.name, work.file.name_len);
                struct CompiledFile *compiled = get_compiled_file(compiler, file_index);

                resolve_top_level(&ctx, &compiled->ast, file_index, work.file.name, work.parent_module);
                break;
            }
        }
    }
    return ctx;
}
