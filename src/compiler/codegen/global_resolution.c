#include "global_resolution.h"
#include "../../parsing/nodes.h"
#include "../module_resolution.h"
#include <string.h>

static void push_global(struct ModuleGlobals *globals, struct Global global)
{
    if (globals->len == globals->cap) {
        u32 new_cap = (globals->cap == 0) ? 2 : globals->cap * 2;
        struct Global *new_ptr = realloc_mem(globals->globals, new_cap * sizeof(struct Global));
        globals->globals = new_ptr;
        globals->cap = new_cap;
    }
    for (u16 i = 0; i < globals->len; i++) {
        struct Global *current = &globals->globals[i];
        if (current->node->name == global.node->name) {
            panic("multiple globals with same name");
        }
    }
    globals->globals[globals->len++] = global;
}

void declare_global_decl(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    bool is_public
) {
    push_global(globals, (struct Global){
        .node = node,
        .constant_index = const_index,
        .is_public = is_public,
    });
}

void init_global_ctx(struct GlobalCtx *globals, struct ModuleCtx *modules)
{
    struct ModuleGlobals *ptr = alloc_mem(modules->modules.count * sizeof(struct ModuleGlobals));
    memset(ptr, 0, modules->modules.count * sizeof(struct ModuleGlobals));

    *globals = (struct GlobalCtx){
        .modules = modules,
        .globals_per_module = ptr,
        .len = modules->modules.count,
    };
}
struct ModuleGlobals *get_module_globals(struct GlobalCtx *globals, u16 mod)
{
    if (mod >= globals->len)
        return null;

    return &globals->globals_per_module[mod];
}


static enum GlobalResolutionError find_global_in(struct ModuleGlobals *mod, const char *ident, bool search_private, u32 *const_index_out)
{
    for (u16 i = 0; i < mod->len; i++) {
        struct Global *current = &mod->globals[i];

        if (current->node->name == ident) {
            if (search_private || current->is_public) {
                *const_index_out = current->constant_index;
                return GLOBAL_RES_OK;
            }
            return GLOBAL_RES_ERROR_PRIVATE;
        }
    }
    return GLOBAL_RES_ERROR_DOESNT_EXIST;
}

enum GlobalResolutionError resolve_global(struct GlobalCtx *ctx, u16 mod, const char *ident, u32 *const_index_out)
{
    return find_global_in(&ctx->globals_per_module[mod], ident, true, const_index_out);
}

struct FindSubmodule {
    u16 module;
    u16 result;
};

static struct FindSubmodule find_submodule_in(struct GlobalCtx *globals, const char *ident, u16 module_index, bool search_private)
{
    struct Module *mod = get_module(globals->modules, module_index);

    for (u16 i = 0; i < mod->items.len; i++) {
        struct ModuleItem *item = &mod->items.ptr[i];

        if (!item->is_submodule) {
            continue;
        }
        if (item->name == ident) {
            if (search_private || item->submodule.is_public) {
                return (struct FindSubmodule){
                    .module = i,
                    .result = GLOBAL_RES_OK,
                };
            } else {
                return (struct FindSubmodule){
                    .result = GLOBAL_RES_ERROR_PRIVATE,
                };
            }
        }
    }
    return (struct FindSubmodule){
        .result = GLOBAL_RES_ERROR_DOESNT_EXIST,
    };
}

static struct GlobalResolutionResult resolve_path(struct GlobalCtx *globals, struct GlobalSearch search, bool is_module)
{
    u16 module_index = search.origin_module;
    struct Module *module = get_module(globals->modules, module_index);
    struct NamespaceAccessNode *searching = search.searching_for;
    bool in_root = true;

    for (;;) {
        if (searching->node.kind == AST_IDENTIFIER) {
            if (is_module) {
                struct IdentifierNode *ident = (struct IdentifierNode*)searching;

                if (ident->src_loc == globals->modules->super_ident) {
                    return (struct GlobalResolutionResult){
                        .error_finding = null,
                        .error = module->parent_index,
                    };
                }

                struct FindSubmodule result = find_submodule_in(globals, ident->src_loc, module_index, in_root);

                if (result.result != GLOBAL_RES_OK) {
                    return (struct GlobalResolutionResult){
                        .error_finding = ident,
                        .error = result.result,
                    };
                }
                u16 submodule = result.module;

                struct ModuleItem *item = &module->items.ptr[submodule];
                if (item->path != null) {
                    return resolve_path(
                        globals,
                        (struct GlobalSearch){
                            .searching_for = (struct NamespaceAccessNode*)item->path,
                            .origin_module = module_index,
                        },
                        true
                    );
                }
                return (struct GlobalResolutionResult){
                    .error_finding = null,
                    .error = item->submodule.index
                };
            } else {
                return (struct GlobalResolutionResult){
                    .error_finding = null,
                    .error = module_index,
                };
            }
        } else {
            if (searching->ident->src_loc == globals->modules->super_ident) {
                if (module->parent_index == (u16)-1) {
                    return (struct GlobalResolutionResult){
                        .error_finding = searching->ident,
                        .error = GLOBAL_RES_ERROR_ROOT_SUPER,
                    };
                }

                module_index = module->parent_index;
                module = get_module(globals->modules, module_index);

                searching = (struct NamespaceAccessNode*)searching->rhs;
                continue;
            }
            struct FindSubmodule result = find_submodule_in(globals, searching->ident->src_loc, module_index, in_root);
            if (result.result != GLOBAL_RES_OK) {
                return (struct GlobalResolutionResult){
                    .error_finding = searching->ident,
                    .error = result.result,
                };
            }
            u16 submodule = result.module;

            struct ModuleItem *item = &module->items.ptr[submodule];

            if (item->path != null) {
                struct GlobalResolutionResult result = resolve_path(
                    globals,
                    (struct GlobalSearch){
                        .searching_for = (struct NamespaceAccessNode*)item->path,
                        .origin_module = module_index,
                    },
                    true
                );
                if (result.error_finding != null) {
                    return result;
                }
                module_index = result.error;
            } else {
                module_index = item->submodule.index;
            }
            if (module_index == (u16)-1)
                panic("unreachable");

            searching = (struct NamespaceAccessNode*)searching->rhs;

            module = get_module(globals->modules, module_index);
            in_root = false;
        }
    }
}

struct GlobalResolutionResult resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    u32 *constant_index_out
) {
    struct GlobalResolutionResult result = resolve_path(globals, search, false);
    if (result.error_finding != null)
        return result;

    u16 module_index = result.error;

    struct NamespaceAccessNode *namespace = search.searching_for;
    while (namespace->node.kind != AST_IDENTIFIER)
        namespace = (struct NamespaceAccessNode*)namespace->rhs;

    u16 origin_parent = get_module(globals->modules, search.origin_module)->parent_index;

    enum GlobalResolutionError item_result = find_global_in(
        &globals->globals_per_module[module_index],
        ((struct IdentifierNode*)namespace)->src_loc,
        origin_parent == module_index || module_index == search.origin_module,
        constant_index_out
    );
    if (item_result == GLOBAL_RES_OK) {
        return (struct GlobalResolutionResult){
            .error_finding = null,
            .error = GLOBAL_RES_OK,
        };
    }
    return (struct GlobalResolutionResult){
        .error_finding = (struct IdentifierNode*)namespace,
        .error = item_result,
    };
}

