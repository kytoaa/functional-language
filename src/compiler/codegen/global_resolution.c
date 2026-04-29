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
            printf(
                "found %.*s, %s\n",
                current->node->name_len,
                current->node->name,
                current->is_public ? "public" : "private"
            );
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

struct IdentifierNode *resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    u32 *constant_index_out
) {
    struct Module *module = &globals->modules->modules.ptr[search.origin_module];

    struct NamespaceAccessNode *searching = search.searching_for;
    bool in_original_mod = true;

    for (;;) {
        bool found_item = false;

        for (u16 i = 0; i < module->items.len; i++) {
            struct ModuleItem *current = &module->items.ptr[i];

            if (current->name == searching->ident->src_loc) {
                if (!current->is_submodule)
                    return searching->ident;
                if (!in_original_mod && !current->submodule.is_public)
                    return searching->ident;

                if (searching->rhs->kind == AST_IDENTIFIER) {
                    if (find_global_in(
                        &globals->globals_per_module[current->submodule.index],
                        ((struct IdentifierNode*)searching->rhs)->src_loc,
                        false,
                        constant_index_out
                    )) {
                        return null;
                    }
                    return ((struct IdentifierNode*)searching->rhs);
                } else {
                    module = &globals->modules->modules.ptr[current->submodule.index];
                    searching = (struct NamespaceAccessNode*)searching->rhs;
                    in_original_mod = false;
                }
                found_item = true;
                break;
            }
        }
        if (!found_item) {
            return searching->ident;
        }
    }

    return searching->ident;
}

