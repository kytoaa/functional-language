#include "module_resolution.h"
#include "../parsing/ast.h"
#include "file_compilation.h"
#include "standard_library.h"

struct ModuleCtxWork {
    struct ModuleCtx ctx;
    struct {
        struct ModuleResolutionWork *ptr;
        u32 len;
        u32 cap;
    } worklist;
};

void free_module_ctx(struct ModuleCtx *modules)
{
    for (u32 i = 0; i < modules->modules.count; i++) {
        free_mem(modules->modules.ptr[i].items.ptr);
    }
    free_mem(modules->modules.ptr);
    free_mem(modules->libraries.ptr);
    *modules = (struct ModuleCtx){};
}

u16 compiled_file_index(const struct Module *module)
{
    return module->_compiled_file_index & 0x3fff;
}
bool is_file_module(const struct Module *module)
{
    return (module->_compiled_file_index & 0x8000) != 0;
}
bool is_type_module(const struct Module *module)
{
    return (module->_compiled_file_index & 0x4000) != 0;
}
inline static void set_compiled_file_index(struct Module *mod, u16 index, bool is_file_mod, bool is_type_mod)
{
    mod->_compiled_file_index = index | (is_file_mod ? 0x8000 : 0) | (is_type_mod ? 0x4000 : 0);
}

static u16 create_module(struct ModuleCtxWork *ctx, const char *name, u32 name_len, u16 parent, struct Location loc)
{
    if (ctx->ctx.modules.count == ctx->ctx.modules.cap) {
        u32 new_cap = (ctx->ctx.modules.cap == 0) ? 2 : ctx->ctx.modules.cap * 2;
        struct Module *new_ptr = realloc_mem(ctx->ctx.modules.ptr, new_cap * sizeof(struct Module));
        ctx->ctx.modules.ptr = new_ptr;
        ctx->ctx.modules.cap = new_cap;
    }
    u16 index = ctx->ctx.modules.count;
    ctx->ctx.modules.ptr[ctx->ctx.modules.count++] = (struct Module){
        .name = name,
        .loc = loc,
        .name_len = name_len,
        .parent_index = parent,
    };
    return index;
}

struct Module *get_module(struct ModuleCtx *ctx, u16 index)
{
    if (index >= ctx->modules.count)
        return null;

    return &ctx->modules.ptr[index];
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

static void queue_resolution_work(struct ModuleCtxWork *ctx, struct ModuleResolutionWork work)
{
    if (ctx->worklist.len == ctx->worklist.cap) {
        u32 new_cap = (ctx->worklist.cap == 0) ? 2 : ctx->worklist.cap * 2;
        struct ModuleResolutionWork *new_ptr = realloc_mem(ctx->worklist.ptr, new_cap * sizeof(struct ModuleResolutionWork));
        ctx->worklist.ptr = new_ptr;
        ctx->worklist.cap = new_cap;
    }
    ctx->worklist.ptr[ctx->worklist.len++] = work;
}

static void queue_file_resolution(struct ModuleCtxWork *ctx, const char *name, u16 name_len, u16 parent_module)
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
static void queue_node_resolution(struct ModuleCtxWork *ctx, struct ModuleDeclNode *node, u16 parent_module)
{
    queue_resolution_work(ctx, (struct ModuleResolutionWork){
        .ast_node = { .node = node },
        .work_kind = MODULE_WORK_AST_NODE,
        .parent_module = parent_module,
    });
}
static bool pop_resolution_queue(struct ModuleCtxWork *ctx, struct ModuleResolutionWork *out)
{
    if (ctx->worklist.len == 0)
        return false;

    *out = ctx->worklist.ptr[--ctx->worklist.len];
    return true;
}

static struct ModuleResult declare_module_items(struct ModuleCtxWork *ctx, struct ModuleDeclNode *module, u16 module_index)
{
    struct Module *mod = get_module(&ctx->ctx, module_index);
    if (module->has_body) {
        struct DeclarationNode *decl = module->declarations;
        while (decl != null) {
            declare_item(mod, (struct ModuleItem){
                .name = decl->name,
                .name_len = decl->name_len,
                .decl_node = decl,
                .is_submodule = false,
            });

            decl = decl->next_declaration;
        }
    }
    struct ModuleDeclNode *submodule = module->submodules;
    while (submodule != null) {
        if (submodule->name == null) {
            return (struct ModuleResult){
                .successful = false,
                .msg = "inline submodules cannot have a self module",
                .file_index = compiled_file_index(mod),
                .location = submodule->node.loc,
            };
        } else if (submodule->has_body == false && submodule->declarations == null) {
            return (struct ModuleResult){
                .successful = false,
                .msg = "inline submodules cannot declare file modules",
                .file_index = compiled_file_index(mod),
                .location = submodule->node.loc,
            };
        } else {
            if (submodule->has_body) {
                declare_item(mod, (struct ModuleItem){
                    .name = submodule->name->src_loc,
                    .name_len = submodule->name->len,
                    .is_submodule = true,
                    .submodule.is_public = true,
                });
                queue_node_resolution(ctx, submodule, module_index);
            } else {
                declare_item(mod, (struct ModuleItem){
                    .name = submodule->name->src_loc,
                    .name_len = submodule->name->len,
                    .path = AS_NODE(submodule->declarations),
                    .is_submodule = true,
                    .submodule.is_public = true,
                });
            }
        }
        submodule = submodule->next_mod;
    }
    return (struct ModuleResult){ .successful = true };
}

static struct ModuleResult resolve_node(struct ModuleCtxWork *ctx, struct ModuleDeclNode *node, u16 parent)
{
    u16 mod_index = create_module(ctx, node->name->src_loc, node->name->len, parent, node->node.loc);
    struct Module *mod = get_module(&ctx->ctx, mod_index);
    struct Module *parent_mod = get_module(&ctx->ctx, parent);

    for (u32 i = 0; i < parent_mod->items.len; i++) {
        struct ModuleItem *item = &parent_mod->items.ptr[i];
        if (item->name == node->name->src_loc) {
            item->submodule.index = mod_index;
            break;
        }
    }

    set_compiled_file_index(mod, compiled_file_index(parent_mod), false, node->is_type);

    return declare_module_items(ctx, node, mod_index);
}

static struct ModuleResult resolve_top_level(
    struct ModuleCtxWork *ctx,
    const struct AstTopLevel *ast,
    u16 file_index,
    const char *mod_name,
    u32 mod_name_len,
    u16 parent
) {
    u16 mod_index = create_module(ctx, mod_name, mod_name_len, parent, (struct Location){});
    struct Module *mod = get_module(&ctx->ctx, mod_index);

    if (parent != (u16)-1) {
        struct Module *parent_mod = get_module(&ctx->ctx, parent);

        for (u32 i = 0; i < parent_mod->items.len; i++) {
            struct ModuleItem *item = &parent_mod->items.ptr[i];
            if (item->name == mod_name) {
                item->submodule.index = mod_index;
                break;
            }
        }
    }

    set_compiled_file_index(mod, file_index, true, false);

    struct ModuleDeclNode *submodule = (struct ModuleDeclNode*)ast->modules;
    while (submodule != null) {
        if (submodule->name == null) {
            mod->loc = submodule->node.loc;
            struct ModuleResult result = declare_module_items(ctx, submodule, mod_index);
            if (!result.successful)
                return result;

            submodule = submodule->next_mod;
            continue;
        }

        struct AstNode *path = null;
        if (!submodule->has_body && submodule->declarations != null)
            path = AS_NODE(submodule->declarations);

        declare_item(mod, (struct ModuleItem){
            .name = submodule->name->src_loc,
            .name_len = submodule->name->len,
            .is_submodule = true,
            .submodule.is_public = false,
            .path = path,
        });
        if (!submodule->has_body && submodule->declarations == null) {
            queue_file_resolution(ctx, submodule->name->src_loc, submodule->name->len, mod_index);
        } else {
            queue_node_resolution(ctx, submodule, mod_index);
        }

        submodule = submodule->next_mod;
    }

    return (struct ModuleResult){
        .successful = true,
        .msg = (const char*)(usize)mod_index,
    };
}

struct ModuleResult resolve_ast(struct Compiler *compiler, const struct CompiledFile *file, struct ModuleCtx *out)
{
    struct ModuleCtxWork ctx = {
        .ctx = {
            .super_ident = ident_table_get(&compiler->identifiers, "super", 5),
            .std_ident = ident_table_get(&compiler->identifiers, "std", 3),
        },
        .worklist = {},
    };
    resolve_top_level(&ctx, &file->ast, 0, "main", 4, (u16)-1);

    ctx.ctx.libraries.ptr = alloc_mem((compiler->config.library_count + 1) * sizeof(struct Library));
    ctx.ctx.libraries.count = compiler->config.library_count + 1;

    {
        const char *lib_name = ctx.ctx.std_ident;

        u32 file_index = compile_module(compiler, lib_name, 3, std_lib_src(), std_lib_src_len());
        if (file_index == (u32)-1) {
            free_mem(ctx.worklist.ptr);
            free_module_ctx(&ctx.ctx);
            return (struct ModuleResult){
                .successful = false,
                .msg = null,
            };
        }
        struct CompiledFile *compiled = get_compiled_file(compiler, file_index);

        struct ModuleResult result = resolve_top_level(&ctx, &compiled->ast, file_index, lib_name, 3, (u16)-1);
        if (!result.successful) {
            free_mem(ctx.worklist.ptr);
            free_module_ctx(&ctx.ctx);
            return result;
        }

        u16 mod_index = (u16)(usize)result.msg;
        ctx.ctx.libraries.ptr[0] = (struct Library){ .name = lib_name, .module_index = mod_index, };
    }

    for (u32 i = 1; i <= compiler->config.library_count; i++) {
        struct LibraryPath *library = &compiler->config.libraries[i];
        u32 file_index = compile_file_module(compiler, null, library->path, library->path_len);
        struct CompiledFile *compiled = get_compiled_file(compiler, file_index);

        const char *lib_name = ident_table_get(&compiler->identifiers, library->name, library->name_len);

        struct ModuleResult result = resolve_top_level(&ctx, &compiled->ast, file_index, library->name, library->name_len, (u16)-1);
        if (!result.successful) {
            free_mem(ctx.worklist.ptr);
            free_module_ctx(&ctx.ctx);
            return result;
        }

        u16 mod_index = (u16)(usize)result.msg;
        ctx.ctx.libraries.ptr[i] = (struct Library){ .name = lib_name, .module_index = mod_index, };
    }

    struct ModuleResolutionWork work = {};
    while (pop_resolution_queue(&ctx, &work)) {
        switch (work.work_kind) {
            case MODULE_WORK_AST_NODE:{
                resolve_node(&ctx, work.ast_node.node, work.parent_module);
                break;
            }
            case MODULE_WORK_FILE:{
                struct CompiledFile *parent = get_compiled_file(compiler, compiled_file_index(get_module(&ctx.ctx, work.parent_module)));

                u32 file_index = compile_file_module(compiler, parent, work.file.name, work.file.name_len);
                if (file_index == (u32)-1) {
                    free_mem(ctx.worklist.ptr);
                    free_module_ctx(&ctx.ctx);
                    return (struct ModuleResult){
                        .msg = null,
                        .successful = false,
                    };
                }
                struct CompiledFile *compiled = get_compiled_file(compiler, file_index);

                resolve_top_level(&ctx, &compiled->ast, file_index, work.file.name, work.file.name_len, work.parent_module);
                break;
            }
        }
    }
    free_mem(ctx.worklist.ptr);
    *out = ctx.ctx;
    return (struct ModuleResult){ .successful = true };
}
