#include "compiler.h"
#include "../parsing/parser.h"
#include "../parsing/nodes.h"
#include "../parsing/debug.h"
#include "../parsing/ident_table.h"
#include "../parsing/traversal.h"

struct FileData {
    const char *src;
    const char *file_name;
    u32 file_name_len;
};

static void print_err(FILE *output, const struct ParseError *err, struct FileData file)
{
    fprintf(output, "error: ");
    if (err->token.type == TOKEN_ERROR) {
        fprintf(output, "%.*s\n", err->token.len, err->token.start);
        fprintf(output, " --> %.*s:%d\n", file.file_name_len, file.file_name, err->token.line);
    } else {
        fprintf(output, "%s\n", err->msg);
        fprintf(
            output,
            " --> %.*s:%d\n  |\n",
            file.file_name_len,
            file.file_name,
            err->token.line
        );
        fprintf(
            output,
            err->token.line > 9 ? "%d| " : "%d | ",
            err->token.line
        );

        u32 line_start_pos = err->token.start - file.src;
        u32 line_end_pos = line_start_pos;
        while (line_start_pos > 0 && file.src[line_start_pos] != '\n') {
            line_start_pos -= 1;
        }
        while (file.src[line_end_pos] != '\n' && file.src[line_end_pos] != '\0') {
            line_end_pos += 1;
        }
        fprintf(output, "%.*s\n  | ", line_end_pos - line_start_pos, file.src + line_start_pos);

        for (const char *i = file.src + line_start_pos; i < err->token.start; i++) {
            fprintf(output, " ");
        }
        for (u32 i = 0; i < err->token.len; i++) {
            fprintf(output, "^");
        }
        fprintf(output, "\n");
    }
    fflush(output);
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

void compile_file(const struct CompilerConfig *config)
{
    struct AstNode *ast;
    struct ParseError parse_err;
    if (!build_ast(config->src, &ast, &parse_err)) {
        print_err(
            config->error,
            &parse_err,
            (struct FileData){
                config->src,
                config->file_name,
                config->file_name_len
            }
        );
        return;
    }

    struct IdentifierTable identifiers;
    init_ident_table(&identifiers);

    traverse_node(ast, &identifiers, generate_symbols, null);

    printf("%.*s", identifiers.items[0].len, identifiers.items[0].ptr);
    for (u32 i = 1; i < identifiers.count; i++) {
        printf(", %.*s", identifiers.items[i].len, identifiers.items[i].ptr);
    }
    printf("\n");

    print_ast(ast);

    free_ident_table(&identifiers);

    return;
}

