#ifndef func_lang_compiler_codegen_global_resolution_h
#define func_lang_compiler_codegen_global_resolution_h

#include "../module_resolution.h"

struct Global {
    struct DeclarationNode *node;
    u32 constant_index;
    u32 uses;
    bool is_public;
};

struct ModuleGlobals {
    struct Global *globals;
    u16 len;
    u16 cap;
    u16 module;
};

struct GlobalCtx {
    struct ModuleCtx *modules;
    struct ModuleGlobals *globals_per_module;
    u16 len;
};

struct GlobalSearch {
    u16 origin_module;
    struct NamespaceAccessNode *searching_for;
};

void init_global_ctx(struct GlobalCtx *globals, struct ModuleCtx *modules);

struct ModuleGlobals *get_module_globals(struct GlobalCtx *globals, u16 mod);

void declare_global_decl(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    bool is_public
);

enum GlobalResolutionError {
    GLOBAL_RES_OK,
    GLOBAL_RES_ERROR_DOESNT_EXIST,
    GLOBAL_RES_ERROR_PRIVATE,
    GLOBAL_RES_ERROR_ROOT_SUPER,
    GLOBAL_RES_ERROR_RECURSION_LIMIT,
};

struct GlobalResolutionResult {
    struct IdentifierNode *error_finding;
    enum GlobalResolutionError error;
};

struct GlobalResolutionResult resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    u32 *constant_index_out
);

enum GlobalResolutionError resolve_global(struct GlobalCtx *ctx, u16 mod, const char *ident, u32 *const_index_out);
u32 global_uses(struct GlobalCtx *ctx, u16 module, const char *ident);


#endif
