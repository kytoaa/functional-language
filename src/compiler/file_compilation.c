#include "file_compilation.h"
#include "error_output.h"
#include "../parsing/traversal.h"
#include "reduction.h"

#include <stdio.h>
#include <string.h>

static void generate_symbols(struct AstNode *node, void *table);
static void fill_symbol_table(struct AstTopLevel *ast, struct IdentifierTable *table);
static void add_compiled_file(struct Compiler *compiler, struct CompiledFile file);

u32 compile_module(
    struct Compiler *compiler,
    const struct CompiledFile *parent,
    const char *module_name,
    u16 module_name_len
) {
    u32 parent_path_len = 0;
    if (parent != null) {
        for (parent_path_len = parent->path_len - 1; parent_path_len >= 0; parent_path_len--) {
            char c = parent->file_path[parent_path_len];
            if (c == '/' || c == '\\') {
                break;
            }
            if (parent_path_len == 0)
                break;
        }
    }
    // allocate enough memory for either path/module.extension or path/module/mod.extension
    char *path = alloc_mem(parent_path_len + module_name_len + 4 + sizeof(FILE_EXTENSION));
    if (parent != null) {
        memcpy(path, parent->file_path, parent_path_len);
    }
    memcpy(path + parent_path_len, module_name, module_name_len);
    memcpy(path + parent_path_len + module_name_len, FILE_EXTENSION, sizeof(FILE_EXTENSION));

    u32 file_name_len = parent_path_len + module_name_len + sizeof(FILE_EXTENSION);

    FILE *file = fopen(path, "rb");
    if (file == null) {
        memcpy(path + parent_path_len + module_name_len, "/mod", 4);
        memcpy(path + parent_path_len + module_name_len + 4, FILE_EXTENSION, sizeof(FILE_EXTENSION));
        file_name_len += 4;
        file = fopen(path, "rb");
    }
    if (file == null) {
        print_module_resolution_error(&compiler->config, path);
        free_mem(path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    rewind(file);

    char *buffer = alloc_mem(file_size + 1);
    usize bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        free_mem(path);
        free_mem(buffer);
        return -1;
    }
    buffer[bytes_read] = '\0';
    fclose(file);

    struct AstTopLevel ast = {};
    struct ParseError parse_err = {};
    if (!build_ast(buffer, &ast, &parse_err)) {
        print_err(
            &compiler->config,
            &parse_err,
            (struct FileData){
                .src = buffer,
                .file_name = path,
                .file_name_len = file_name_len,
            }
        );
        free_mem(path);
        free_mem(buffer);
        return -1;
    }

    fill_symbol_table(&ast, &compiler->identifiers);

    reduce_ast(&ast);

    add_compiled_file(compiler, (struct CompiledFile){
        .ast = ast,
        .file_path = path,
        .src = buffer,
        .src_len = file_size,
        .path_len = file_name_len,
    });

    return compiler->files.count - 1;
}

struct CompiledFile *get_compiled_file(struct Compiler *compiler, u32 file)
{
    if (file >= compiler->files.count)
        return null;

    return &compiler->files.ptr[file];
}

static void fill_symbol_table(struct AstTopLevel *ast, struct IdentifierTable *table)
{
    struct DeclarationNode *decl = (struct DeclarationNode*)ast->declarations;
    while (decl != null) {
        traverse_node(AS_NODE(decl), table, generate_symbols, null);
        decl = decl->next_declaration;
    }
    struct ModuleDeclNode *module = (struct ModuleDeclNode*)ast->modules;
    while (module != null) {
        traverse_node(AS_NODE(module), table, generate_symbols, null);
        module = module->next_mod;
    }
}

static void generate_symbols(struct AstNode *node, void *table)
{
    struct IdentifierTable *identifiers = table;

    switch (node->kind) {
        case AST_IDENTIFIER:{
            struct IdentifierNode *ident = (struct IdentifierNode*)node;
            ident->src_loc = ident_table_get(identifiers, ident->src_loc, ident->len);
            break;
        }
        case AST_DECLARATION:{
            struct DeclarationNode *decl = (struct DeclarationNode*)node;
            decl->name = ident_table_get(identifiers, decl->name, decl->name_len);
            break;
        }
        case AST_FUNCTION_BINDING:{
            struct FunctionBindingNode *binding = (struct FunctionBindingNode*)node;
            binding->src_loc = ident_table_get(identifiers, binding->src_loc, binding->len);
            break;
        }
        default:
            break;
    }
}

static void add_compiled_file(struct Compiler *compiler, struct CompiledFile file)
{
    if (compiler->files.count == compiler->files.cap) {
        u32 new_cap = (compiler->files.cap == 0) ? 1 : compiler->files.cap * 2;
        struct CompiledFile *new_ptr = realloc_mem(compiler->files.ptr, new_cap * sizeof(struct CompiledFile));
        compiler->files.ptr = new_ptr;
        compiler->files.cap = new_cap;
    }

    compiler->files.ptr[compiler->files.count++] = file;
}

