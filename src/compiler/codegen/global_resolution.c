#include "global_resolution.h"
#include "../../parsing/nodes.h"
#include "../module_resolution.h"
#include <stdio.h>
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


static bool find_global_in(struct ModuleGlobals *mod, const char *ident, bool search_private, u32 *const_index_out)
{
    for (u16 i = 0; i < mod->len; i++) {
        struct Global *current = &mod->globals[i];

        if (current->node->name == ident) {
            if (search_private || current->is_public) {
                *const_index_out = current->constant_index;
                return true;
            }
            return false;
        }
    }
    return false;
}

bool resolve_global(struct GlobalCtx *ctx, u16 mod, const char *ident, u32 *const_index_out)
{
    return find_global_in(&ctx->globals_per_module[mod], ident, true, const_index_out);
}

static u16 find_submodule_in(struct GlobalCtx *globals, const char *ident, u16 module_index, bool search_private)
{
    struct Module *mod = get_module(globals->modules, module_index);

    for (u16 i = 0; i < mod->items.len; i++) {
        struct ModuleItem *item = &mod->items.ptr[i];

        if (!item->is_submodule) {
            printf("%.*s is not a module\n", item->name_len, item->name);
            continue;
        }
        if (item->name == ident) {
            if (search_private || item->submodule.is_public) {
                return i;
            } else {
                printf("private\n");
                return -1;
            }
        }
    }
    return -1;
}

static u16 resolve_path(struct GlobalCtx *globals, struct GlobalSearch search, bool is_module)
{
    u16 module_index = search.origin_module;
    struct Module *module = get_module(globals->modules, module_index);
    struct NamespaceAccessNode *searching = search.searching_for;
    bool in_root = true;

    for (;;) {
        if (searching->node.kind == AST_IDENTIFIER) {
            if (is_module) {
                struct IdentifierNode *ident = (struct IdentifierNode*)searching;
                printf("searching for %.*s in %d\n", ident->len, ident->src_loc, module_index);
                u16 submodule = find_submodule_in(globals, ident->src_loc, module_index, in_root);

                if (submodule == (u16)-1)
                    return -1;

                printf("exists\n");

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
                printf("found it\n");
                return item->submodule.index;
            } else {
                return module_index;
            }
        } else {
            printf("searching for submodule %.*s in %d\n", searching->ident->len, searching->ident->src_loc, module_index);
            u16 submodule = find_submodule_in(globals, searching->ident->src_loc, module_index, in_root);
            if (submodule == (u16)-1)
                return -1;

            struct ModuleItem *item = &module->items.ptr[submodule];

            if (item->path != null) {
                printf("%d has path, resolving\n", submodule);
                module_index = resolve_path(
                    globals,
                    (struct GlobalSearch){
                        .searching_for = (struct NamespaceAccessNode*)item->path,
                        .origin_module = module_index,
                    },
                    true
                );
                printf("found at module %d\n", module_index);
            } else {
                module_index = item->submodule.index;
            }
            if (module_index == (u16)-1)
                return -1;

            searching = (struct NamespaceAccessNode*)searching->rhs;

            module = get_module(globals->modules, module_index);
            in_root = false;
        }
    }
}

struct IdentifierNode *resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    u32 *constant_index_out
) {
    u16 module_index = resolve_path(globals, search, false);
    if (module_index == (u16)-1)
        return search.searching_for->ident;

    printf("returned module %d\n", module_index);

    struct NamespaceAccessNode *namespace = search.searching_for;
    while (namespace->node.kind != AST_IDENTIFIER)
        namespace = (struct NamespaceAccessNode*)namespace->rhs;

    if (find_global_in(
        &globals->globals_per_module[module_index],
        ((struct IdentifierNode*)namespace)->src_loc,
        false,
        constant_index_out
    )) {
        return null;
    }
    return (struct IdentifierNode*)namespace;
}

