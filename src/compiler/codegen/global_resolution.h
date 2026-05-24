#ifndef func_lang_compiler_codegen_global_resolution_h
#define func_lang_compiler_codegen_global_resolution_h

#include "../module_resolution.h"

struct Global {
    struct DeclarationNode *node;
    u32 constant_index;
    u16 closure_info_index;
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


void init_global_ctx(struct GlobalCtx *globals, struct ModuleCtx *modules);
void free_global_ctx(struct GlobalCtx *globals);

struct ModuleGlobals *get_module_globals(struct GlobalCtx *globals, u16 mod);

void declare_global_decl(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    u16 closure_info_index,
    bool is_public
);
void set_global_decl_const_index(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    u16 closure_info_index
);

struct GlobalResolutionResult resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    u32 *constant_index_out,
    u16 *closure_index_out
);
struct GlobalResolutionResult find_global_decl(
    struct GlobalCtx *globals,
    struct AstNode *search_for,
    u16 search_from_module,
    struct DeclarationNode **out,
    u16 *out_mod
);

enum GlobalResolutionError resolve_global(struct GlobalCtx *ctx, u16 mod, const char *ident, u32 *const_index_out, u16 *closure_index_out);

struct GlobalResolutionResult get_module_index_for(struct GlobalCtx *globals, struct GlobalSearch search);

#endif
