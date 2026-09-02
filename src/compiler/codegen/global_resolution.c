#include "global_resolution.h"
#include "../../parsing/nodes.h"
#include "../module_resolution.h"
#include <string.h>

#define PATH_RESOLUTION_RECURSION_LIMIT 32

static struct GlobalResolutionResult resolve_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    bool is_module,
    u16 current_depth
);

static enum GlobalResolutionError resolve_global_inner(
    struct GlobalCtx *ctx,
    u16 mod,
    const char *ident,
    struct GlobalInfo *out_info,
    bool search_private
);

static enum GlobalDeclError push_global(
    struct ModuleGlobals *globals,
    struct Global global,
    struct Location *out_err_loc
) {
    if (globals->len == globals->cap) {
        u32 new_cap = (globals->cap == 0) ? 2 : globals->cap * 2;
        struct Global *new_ptr = realloc_mem(globals->globals, new_cap * sizeof(struct Global));
        globals->globals = new_ptr;
        globals->cap = new_cap;
    }
    for (u16 i = 0; i < globals->len; i++) {
        struct Global *current = &globals->globals[i];
        if (current->node->name == global.node->name) {
            *out_err_loc = current->node->node.loc;
            return GLOBAL_DECL_ERROR_REDECLARED;
        }
    }
    globals->globals[globals->len++] = global;
    return GLOBAL_DECL_OK;
}

enum GlobalDeclError declare_global_decl(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    u16 closure_info_index,
    bool is_public,
    bool is_type_module,
    struct Location *out_err_loc
) {
    bool is_constructor = false;
    u8 arg_count = 0;
    if (node->body->kind == AST_CONSTRUCTOR) {
        is_constructor = true;
        if (((struct ConstructorNode*)node->body)->body != null)
            arg_count = -1;
    } else if (node->body->kind == AST_LAMBDA) {
        struct LambdaNode *lambda = (struct LambdaNode*)node->body;
        is_constructor = lambda->body->kind == AST_CONSTRUCTOR;

        struct FunctionBindingNode *binding = lambda->bindings;
        while (binding != null) {
            binding = binding->next_binding;
            arg_count += 1;
        }
    }
    // if arg_count != -1 then a constructor, not an import
    if (is_constructor && arg_count != (u8)-1 && !is_type_module) {
        return GLOBAL_DECL_ERROR_MOD_NOT_TYPE;
    }

    return push_global(
        globals,
        (struct Global){
            .node = node,
            .constant_index = const_index,
            .is_constructor = is_constructor,
            .variant_index = -1,
            .is_public = is_public,
        },
        out_err_loc
    );
}
void set_global_decl_info(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    struct GlobalInfo info
) {
    for (u32 i = 0; i < globals->len; i++) {
        struct Global *global = &globals->globals[i];
        if (global->node == node) {
            global->constant_index = info.constant_index;
            global->variant_index = info.variant_index;
            global->arg_count = info.arg_count;
            return;
        }
    }
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
void free_global_ctx(struct GlobalCtx *globals)
{
    for (u32 i = 0; i < globals->len; i++) {
        free_mem(globals->globals_per_module[i].globals);
    }
    free_mem(globals->globals_per_module);
    globals->len = 0;
    globals->globals_per_module = null;
}

struct ModuleGlobals *get_module_globals(struct GlobalCtx *globals, u16 mod)
{
    if (mod >= globals->len)
        return null;

    return &globals->globals_per_module[mod];
}


static enum GlobalResolutionError find_global_in(
    struct ModuleGlobals *mod,
    const char *ident,
    bool search_private,
    struct GlobalInfo *out_info
) {
    for (u16 i = 0; i < mod->len; i++) {
        struct Global *current = &mod->globals[i];

        if (current->node->name == ident) {
            if (search_private || current->is_public) {
                if (out_info != null) {
                    *out_info = (struct GlobalInfo){
                        .constant_index = current->constant_index,
                        .variant_index = current->variant_index,
                        .arg_count = current->arg_count,
                        .is_constructor = current->is_constructor,
                    };
                }
                return GLOBAL_RES_OK;
            }
            return GLOBAL_RES_ERROR_PRIVATE;
        }
    }
    return GLOBAL_RES_ERROR_DOESNT_EXIST;
}

static enum GlobalResolutionError resolve_global_inner(
    struct GlobalCtx *ctx,
    u16 mod,
    const char *ident,
    struct GlobalInfo *out_info,
    bool search_private
) {
    return find_global_in(
        &ctx->globals_per_module[mod],
        ident,
        search_private,
        out_info
    );
}

enum GlobalResolutionError resolve_global(
    struct GlobalCtx *ctx,
    u16 mod,
    const char *ident,
    struct GlobalInfo *out_info
) {
    return resolve_global_inner(ctx, mod, ident, out_info, true);
}

struct FindSubmodule {
    u16 module;
    u16 result;
};

static struct FindSubmodule find_submodule_in(
    struct GlobalCtx *globals,
    const char *ident,
    u16 module_index,
    bool search_private
) {
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
static bool find_library(struct GlobalCtx *globals, const char *name, u16 *out)
{
    for (u16 i = 0; i < globals->modules->libraries.count; i++) {
        struct Library *library = &globals->modules->libraries.ptr[i];
        if (library->name == name) {
            *out = library->module_index;
            return true;
        }
    }
    return false;
}

static struct GlobalResolutionResult resolve_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    bool is_module,
    u16 current_depth
) {
    if (current_depth >= PATH_RESOLUTION_RECURSION_LIMIT) {
        return (struct GlobalResolutionResult){
            .error_finding = search.searching_for->ident,
            .error = GLOBAL_RES_ERROR_RECURSION_LIMIT,
        };
    }
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
                    u16 module_index = 0;
                    if (find_library(globals, ident->src_loc, &module_index)) {
                        return (struct GlobalResolutionResult){
                            .error_finding = null,
                            .error = module_index,
                        };
                    }
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
                        true,
                        current_depth + 1
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
                u16 module = 0;
                if (find_library(globals, searching->ident->src_loc, &module)) {
                    module_index = module;
                    goto found_module;
                } else {
                    return (struct GlobalResolutionResult){
                        .error_finding = searching->ident,
                        .error = result.result,
                    };
                }
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
                    true,
                    current_depth + 1
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

        found_module:
            searching = (struct NamespaceAccessNode*)searching->rhs;

            module = get_module(globals->modules, module_index);
            in_root = false;
        }
    }
}

struct GlobalResolutionResult get_module_index_for(struct GlobalCtx *globals, struct GlobalSearch search)
{
    return resolve_path(globals, search, false, 0);
}

struct GlobalResolutionResult resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    struct GlobalInfo *out_info
) {
    struct GlobalResolutionResult result = resolve_path(globals, search, false, 0);
    if (result.error_finding != null)
        return result;

    u16 module_index = result.error;

    struct NamespaceAccessNode *namespace = search.searching_for;
    while (namespace->node.kind != AST_IDENTIFIER)
        namespace = (struct NamespaceAccessNode*)namespace->rhs;

    u16 parent_index = get_module(globals->modules, search.origin_module)->parent_index;

    enum GlobalResolutionError item_result = find_global_in(
        &globals->globals_per_module[module_index],
        ((struct IdentifierNode*)namespace)->src_loc,
        module_index == parent_index || module_index == search.origin_module,
        out_info
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

struct GlobalResolutionResult find_global_decl(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    struct DeclarationNode **out,
    u16 *out_mod
) {
    const struct AstNode *const search_for = AS_NODE(search.searching_for);
    const u16 search_from_module = search.origin_module;

    struct IdentifierNode *ident = null;
    u16 module_index = 0;
    switch (search_for->kind) {
        case AST_IDENTIFIER:{
            module_index = search_from_module;
            ident = (struct IdentifierNode*)search_for;
            break;
        }
        case AST_NAMESPACE_ACCESS:{
            struct GlobalResolutionResult result = resolve_path(
                globals,
                (struct GlobalSearch){
                    .searching_for = (struct NamespaceAccessNode*)search_for,
                    .origin_module = search_from_module,
                },
                false,
                0
            );
            if (result.error_finding != null)
                return result;

            struct NamespaceAccessNode *node = (struct NamespaceAccessNode*)search_for;
            while (node->node.kind == AST_NAMESPACE_ACCESS) {
                node = (struct NamespaceAccessNode*)node->rhs;
            }
            ident = (struct IdentifierNode*)node;

            module_index = result.error;
            break;
        }
        default:
            panic("unreachable, not an identifier or namespace access");
    }

    struct ModuleGlobals *module_globals = get_module_globals(globals, module_index);

    for (u32 i = 0; i < module_globals->len; i++) {
        struct Global *global = &module_globals->globals[i];
        if (global->node->name == ident->src_loc) {
            if (out != null)
                *out = global->node;
            if (out_mod != null)
                *out_mod = module_index;
            return (struct GlobalResolutionResult){
                .error = GLOBAL_RES_OK,
                .error_finding = null,
            };
        }
    }

    return (struct GlobalResolutionResult){
        .error = GLOBAL_RES_ERROR_DOESNT_EXIST,
        .error_finding = ident,
    };
}

