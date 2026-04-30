#include "error_output.h"
#include "codegen/codegen.h"
#include "compiler.h"
#include "../parsing/ast.h"
#include "../lexer.h"
#include "file_compilation.h"
#include <stdio.h>

struct LineIdentPrintInfo {
    /// index within the file
    u32 line_start;
    u32 line_len;
    /// index within the line
    u32 ident_start;
    u32 ident_len;
};

static inline bool eol_char(char c)
{
    return c == EOF_CHAR || c == '\n';
}
static inline bool ident_char(char c)
{
    return is_alpha(c) || is_digit(c);
}

static struct LineIdentPrintInfo ident_info(const char *src, struct Location loc)
{
    i32 line_start_index = loc.file_pos;
    u32 line_end_index = loc.file_pos;
    u32 ident_len = 0;

    while (line_start_index >= 0 && src[line_start_index] != '\n') {
        line_start_index -= 1;
    }
    line_start_index += 1;
    while (ident_char(src[line_end_index])) {
        line_end_index += 1;
        ident_len += 1;
    }
    while (!eol_char(src[line_end_index])) {
        line_end_index += 1;
    }
    line_end_index -= 1;

    return (struct LineIdentPrintInfo){
        .line_start = line_start_index,
        .line_len = line_end_index - line_start_index,
        .ident_start = loc.file_pos - line_start_index,
        .ident_len = ident_len,
    };
}

static void print_ident(FILE *err, const char *src, struct Location loc)
{
    struct LineIdentPrintInfo ident = ident_info(src, loc);
    fprintf(err, "    |\n%-4d| ", loc.line);
    fprintf(err, "%.*s\n", ident.line_len, &src[ident.line_start]);
    fprintf(err, "    | ");
    for (u32 i = 0; i < ident.ident_start; i++) {
        fprintf(err, " ");
    }
    for (u32 i = 0; i < ident.ident_len; i++) {
        fprintf(err, "^");
    }
    fprintf(err, "\n");
}

void print_codegen_error(struct Compiler *compiler, struct CodegenError error)
{
    const char *src = get_compiled_file(compiler, error.file_index)->src;
    const struct CompilerConfig *config = &compiler->config;
    switch (error.type) {
        case CODEGEN_ERR_NON_EXISTENT_IDENT:{
            fprintf(
                config->error,
                "error: non existent identifier\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.non_existent_identifier.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.non_existent_identifier.loc
            );
            break;
        }
        case CODEGEN_ERR_USED_UNDERSCORE:{
            fprintf(
                config->error,
                "error: '_' is not a valid identifier\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.used_underscore.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.used_underscore.loc
            );
            break;
        }
        case CODEGEN_ERR_REDECLARED_GLOBAL:{
            fprintf(
                config->error,
                "error: redeclared global\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.redeclared_global.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.redeclared_global.loc
            );
            fprintf(
                config->error,
                "previously declared here:\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.redeclared_global.prev_decl_loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.redeclared_global.prev_decl_loc
            );
            break;
        }
        case CODEGEN_ERR_MULTIPLE_MAIN_DECL:{
            fprintf(
                config->error,
                "error: main declared multiple times\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.redeclared_global.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.redeclared_global.loc
            );
            fprintf(
                config->error,
                "previously declared here:\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.redeclared_global.prev_decl_loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.redeclared_global.prev_decl_loc
            );
            break;
        }
        case CODEGEN_ERR_MAIN_ARGS:{
            fprintf(
                config->error,
                "error: main cannot have arguments\n --> %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.main_args.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.main_args.loc
            );
            break;
        }
    }
    if (error.additional_msg != null) {
        fprintf(config->error, "note: %s\n", error.additional_msg);
    }
}

void print_err(const struct CompilerConfig *config, const struct ParseError *err, struct FileData file)
{
    FILE *output = config->error;

    fprintf(output, "error: ");
    if (err->token.type == TOKEN_ERROR) {
        fprintf(output, "%.*s\n", err->token.len, err->token.start);
        fprintf(output, " --> %.*s:%d\n", file.file_name_len, file.file_name, err->token.line);
    } else {
        fprintf(output, "%s\n", err->msg);
        fprintf(
            output,
            " --> %.*s:%d\n",
            file.file_name_len,
            file.file_name,
            err->token.line
        );
        if (err->token.start == null || file.src == null)
            return;
        fprintf(output, "  |\n");
        fprintf(
            output,
            err->token.line > 9 ? "%d| " : "%d | ",
            err->token.line
        );

        u32 line_start_pos = err->token.start - file.src;
        u32 line_end_pos = line_start_pos;
        while (line_start_pos > 1 && file.src[line_start_pos - 1] != '\n') {
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

void print_module_resolution_error(const struct CompilerConfig *config, const char *module_name)
{
    fprintf(config->error, "could not locate file '%s'\n", module_name);
}
