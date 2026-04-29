#ifndef func_lang_compiler_file_compilation_h
#define func_lang_compiler_file_compilation_h

#include "compiler.h"
#include "../bytecode.h"
#include "../parsing/ast.h"
#include "../parsing/ident_table.h"

struct CompiledFile {
    struct AstTopLevel ast;
    const char *file_path;
    const char *src;
    u32 src_len;
    u16 path_len;
};

struct Compiler {
    struct CompilerConfig config;
    struct IdentifierTable identifiers;
    struct Chunk chunk;

    struct {
        struct CompiledFile *ptr;
        u16 count;
        u16 cap;
    } files;
};

u32 compile_module(
    struct Compiler *compiler,
    const struct CompiledFile *parent,
    const char *module_name,
    u16 module_name_len
);

struct CompiledFile *get_compiled_file(struct Compiler *compiler, u32 file);

#define FILE_EXTENSION ".fl"

#endif
