#ifndef func_lang_compiler_error_output_h
#define func_lang_compiler_error_output_h

#include "compiler.h"
#include "codegen/codegen.h"
#include "file_compilation.h"
#include "../parsing/parser.h"

void print_codegen_error(struct Compiler *compiler, struct CodegenError error);
void print_err(const struct CompilerConfig *config, const struct ParseError *err, struct FileData file);
/// `module_name` is null terminated
void print_module_resolution_error(const struct CompilerConfig *config, const char *module_name);

#endif
