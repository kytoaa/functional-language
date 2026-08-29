#ifndef func_lang_compiler_module_resolution_h
#define func_lang_compiler_module_resolution_h

#include "../prelude.h"
#include "../parsing/nodes.h"
#include "file_compilation.h"

struct ModuleItem {
    const char *name;
    struct AstNode *path;
    union {
        struct DeclarationNode *decl_node;
        struct {
            u16 index;
            bool is_public;
        } submodule;
    };
    u16 name_len;
    bool is_submodule;
};

struct Module {
    const char *name;
    struct Location loc;
    struct {
        struct ModuleItem *ptr;
        u32 len;
        u32 cap;
    } items;
    struct {
        struct AstNode **ptr;
        u32 len;
        u32 cap;
    } use_decls;
    u32 name_len;
    u16 _compiled_file_index;
    u16 parent_index;
};
u16 compiled_file_index(const struct Module *module);
bool is_file_module(const struct Module *module);
bool is_type_module(const struct Module *module);

struct Library {
    const char *name;
    u32 module_index;
};

struct ModuleResult {
    const char *msg;
    struct Location location;
    u16 file_index;
    bool successful;
};

enum ModuleWorkKind {
    MODULE_WORK_AST_NODE,
    MODULE_WORK_FILE,
};

struct ModuleResolutionWork {
    union {
        struct {
            struct ModuleDeclNode *node;
        } ast_node;
        struct {
            const char *name;
            u16 name_len;
        } file;
    };
    enum ModuleWorkKind work_kind;
    u16 parent_module;
};

struct ModuleCtx {
    struct {
        struct Module *ptr;
        u32 count;
        u32 cap;
    } modules;
    struct {
        struct Library *ptr;
        u32 count;
    } libraries;

    const char *super_ident;
    const char *std_ident;
};

struct ModuleResult resolve_ast(struct Compiler *compiler, const struct CompiledFile *file, struct ModuleCtx *out);
struct Module *get_module(struct ModuleCtx *modules, u16 index);

void free_module_ctx(struct ModuleCtx *modules);

#endif
