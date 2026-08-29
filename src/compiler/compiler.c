#include "compiler.h"
#include "../vm/vm.h"
#include "../parsing/ident_table.h"
#include "codegen/codegen.h"
#include "codegen/top_level.h"
#include "error_output.h"
#include "file_compilation.h"
#include "module_resolution.h"

static void run_chunk(const struct CompilerConfig *config, struct Chunk chunk)
{
    /*for (u32 i = 0; i < chunk.closures.len; i++) {
        struct ClosureInfo closure = chunk.closures.ptr[i];
        printf("{ addr: %d, arity: %d, captures: %d }\n", closure.address, closure.arity, closure.capture_count);
    }*/
    //print_instructions(config->output, &chunk);

    run_vm(&chunk, (struct VmConfig){ .out = config->output, .error = config->error });
}

void compile_file(const struct CompilerConfig config)
{
    struct Compiler compiler = { .config = config };
    init_ident_table(&compiler.identifiers);
    init_chunk(&compiler.chunk);

    const char *file_extension = config.file_name + config.file_name_len - 1;
    while (file_extension > config.file_name && *file_extension != '.') {
        file_extension -= 1;
    }
    if (file_extension - config.file_name != config.file_name_len - (sizeof(FILE_EXTENSION) - 1)) {
        panic("input error, not a source file\n");
    }
    for (u32 i = 0; i < sizeof(FILE_EXTENSION) - 2; i++) {
        if (file_extension[i + 1] != FILE_EXTENSION[i + 1])
            panic("input error, not a source file\n");
    }
    const u32 file_name_len = config.file_name_len - (sizeof(FILE_EXTENSION) - 1);

    u32 result = compile_file_module(&compiler, null, config.file_name, file_name_len);
    //print_ast(&compiler.files.ptr[0].ast);
    if (result == (u32)-1) {
        free_compiler(&compiler);
        free_chunk(&compiler.chunk);
        return;
    }

    struct ModuleCtx modules = {};
    struct ModuleResult mod_result = resolve_ast(&compiler, &compiler.files.ptr[0], &modules);
    if (!mod_result.successful) {
        if (mod_result.msg != null) {
            print_codegen_error(
                &compiler,
                make_message_error(mod_result.location, mod_result.msg, mod_result.file_index)
            );
        }
        free_module_ctx(&modules);
        free_compiler(&compiler);
        free_chunk(&compiler.chunk);
        return;
    }

    struct CodegenErrorList errors = generate_code(&compiler, &modules);

    if (errors.len > 0) {
        for (u32 i = 0; i < errors.len; i++) {
            print_codegen_error(&compiler, errors.ptr[i]);
        }
        free_codegen_errors(&errors);
    }

    free_module_ctx(&modules);
    free_compiler(&compiler);

    free_ast();

    if (errors.len == 0) {
        run_chunk(&config, compiler.chunk);
    }

    free_chunk(&compiler.chunk);

    return;
}

