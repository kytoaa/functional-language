#ifndef func_lang_compiler_module_resolution_h
#define func_lang_compiler_module_resolution_h

#include "../prelude.h"
#include "../parsing/nodes.h"
#include "file_compilation.h"

struct ModuleItem {
    const char *name;
    u16 name_len;
    u16 submodule_index;
    bool is_submodule;
};

struct Module {
    const char *name;
    struct {
        struct ModuleItem *ptr;
        u32 len;
        u32 cap;
    } items;
    u16 compiled_file_index;
    u16 parent_index;
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
        struct ModuleResolutionWork *ptr;
        u32 len;
        u32 cap;
    } worklist;
};

struct ModuleCtx resolve_ast(struct Compiler *compiler, const struct CompiledFile *file);

#endif
