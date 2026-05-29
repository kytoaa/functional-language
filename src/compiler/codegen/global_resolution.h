#ifndef func_lang_compiler_codegen_global_resolution_h
#define func_lang_compiler_codegen_global_resolution_h

#include "../module_resolution.h"

struct Global {
    struct DeclarationNode *node;
    u32 constant_index;
    u16 variant_index;
    u8 arg_count;
    struct {
        bool is_public : 1;
        bool is_constructor : 1;
    };
};
struct GlobalInfo {
    u32 constant_index;
    u16 variant_index;
    u8 arg_count;
    bool is_constructor;
};

struct ModuleGlobals {
    struct Global *globals;
    u16 len;
    u16 cap;
    u16 module;
    u16 type_info_index;
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

enum GlobalDeclError {
    GLOBAL_DECL_OK,
    GLOBAL_DECL_ERROR_REDECLARED,
    GLOBAL_DECL_ERROR_MOD_NOT_TYPE,
};

enum GlobalDeclError declare_global_decl(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    u32 const_index,
    u16 closure_info_index,
    bool is_public,
    bool is_type_module,
    struct Location *out_err_loc
);
void set_global_decl_info(
    struct ModuleGlobals *globals,
    struct DeclarationNode *node,
    struct GlobalInfo info
);

struct GlobalResolutionResult resolve_global_path(
    struct GlobalCtx *globals,
    struct GlobalSearch search,
    struct GlobalInfo *out_info
);
struct GlobalResolutionResult find_global_decl(
    struct GlobalCtx *globals,
    struct AstNode *search_for,
    u16 search_from_module,
    struct DeclarationNode **out,
    u16 *out_mod
);

enum GlobalResolutionError resolve_global(struct GlobalCtx *ctx, u16 mod, const char *ident, struct GlobalInfo *out_info);

struct GlobalResolutionResult get_module_index_for(struct GlobalCtx *globals, struct GlobalSearch search);

#endif
