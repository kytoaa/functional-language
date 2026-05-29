#include "error_output.h"
#include "codegen/codegen.h"
#include "compiler.h"
#include "../parsing/ast.h"
#include "../lexer.h"
#include "file_compilation.h"
#include <stdio.h>

#define ERROR_STR "\x1b[1;31merror\x1b[0m:"
#define ARROW_STR "\x1b[1;34m-->\x1b[0m"

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
    if (ident_len == 0)
        ident_len = 1;

    while (!eol_char(src[line_end_index])) {
        line_end_index += 1;
    }
    line_end_index -= 1;

    return (struct LineIdentPrintInfo){
        .line_start = line_start_index,
        .line_len = (line_end_index - line_start_index) + 1,
        .ident_start = loc.file_pos - line_start_index,
        .ident_len = ident_len,
    };
}

static void print_ident(FILE *err, const char *src, struct Location loc)
{
    struct LineIdentPrintInfo ident = ident_info(src, loc);
    fprintf(err, "\x1b[1;34m    |\n%-4d|\x1b[0m ", loc.line);
    fprintf(
        err,
        "%.*s\x1b[32m%.*s\x1b[0m%.*s\n",
        ident.ident_start,
        &src[ident.line_start],
        ident.ident_len,
        &src[ident.line_start + ident.ident_start],
        ident.line_len - (ident.ident_start + ident.ident_len),
        &src[ident.line_start + ident.ident_start + ident.ident_len]
    );
    fprintf(err, "    \x1b[1;34m|\x1b[0m ");
    for (u32 i = 0; i < ident.ident_start; i++) {
        if (src[ident.line_start + i] == '\t') {
            fprintf(err, "\t");
        } else {
            fprintf(err, " ");
        }
    }
    fprintf(err, "\x1b[32m");
    for (u32 i = 0; i < ident.ident_len; i++) {
        fprintf(err, "^");
    }
    fprintf(err, "\x1b[0m\n");
}

void print_codegen_error(struct Compiler *compiler, struct CodegenError error)
{
    const char *src = get_compiled_file(compiler, error.file_index)->src;
    const struct CompilerConfig *config = &compiler->config;
    switch (error.type) {
        case CODEGEN_ERR_NON_EXISTENT_IDENT:{
            fprintf(
                config->error,
                ERROR_STR " non existent identifier\n "ARROW_STR" %.*s:%d\n",
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
                ERROR_STR " '_' is not a valid identifier\n "ARROW_STR" %.*s:%d\n",
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
                ERROR_STR " redeclared global\n "ARROW_STR" %.*s:%d\n",
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
                "previously declared here:\n "ARROW_STR" %.*s:%d\n",
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
                ERROR_STR " main declared multiple times\n "ARROW_STR" %.*s:%d\n",
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
                "previously declared here:\n "ARROW_STR" %.*s:%d\n",
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
                ERROR_STR " main cannot have arguments\n "ARROW_STR" %.*s:%d\n",
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
        case CODEGEN_ERR_INVALID_PATTERN:{
            fprintf(
                config->error,
                ERROR_STR " not a valid pattern\n "ARROW_STR" %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.invalid_pattern.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.invalid_pattern.loc
            );
            break;
        }
        case CODEGEN_ERR_MOD_NOT_TYPE:{
            fprintf(
                config->error,
                ERROR_STR " constructor declared in module that isnt a type\n "ARROW_STR" %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.mod_not_type.loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.mod_not_type.loc
            );
            fprintf(
                config->error,
                "module declared in:\n "ARROW_STR" %.*s:%d\n",
                config->file_name_len, config->file_name,
                error.error.mod_not_type.mod_loc.line
            );
            print_ident(
                config->error,
                src,
                error.error.mod_not_type.mod_loc
            );
            break;
        }
        case CODEGEN_ERR_MSG:{
            fprintf(
                config->error,
                ERROR_STR " %s\n "ARROW_STR" %.*s:%d\n",
                error.error.with_message.msg,
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
        fprintf(config->error, "\x1b[1;37mnote\x1b[0m: %s\n", error.additional_msg);
    }
    fflush(config->error);
}

void print_err(const struct CompilerConfig *config, const struct ParseError *err, struct FileData file)
{
    if (err->token.type == TOKEN_ERROR) {
        fprintf(
            config->error,
            ERROR_STR " %.*s\n "ARROW_STR" %.*s:%d\n",
            err->token.len,
            err->token.start,
            file.file_name_len,
            file.file_name,
            err->token.line
        );
    } else {
        fprintf(
            config->error,
            ERROR_STR " %s\n "ARROW_STR" %.*s:%d\n",
            err->msg,
            file.file_name_len,
            file.file_name,
            err->token.line
        );
        print_ident(
            config->error,
            file.src,
            (struct Location){
                .line = err->token.line,
                .file_pos = err->token.start - file.src
            }
        );
    }
    fflush(config->error);
}

void print_module_resolution_error(const struct CompilerConfig *config, const char *module_name)
{
    fprintf(config->error, "could not locate file '%s'\n", module_name);
}
