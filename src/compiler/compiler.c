#include "compiler.h"
#include "../vm/vm.h"
#include "../parsing/ident_table.h"
#include "codegen/codegen.h"
#include "codegen/top_level.h"
#include "error_output.h"
#include "file_compilation.h"
#include "module_resolution.h"
#include "../compiler_info.h"
#include "../bytecode/binary.h"

static void run_chunk(const struct CompilerConfig *config, struct Chunk chunk)
{
    /*for (u32 i = 0; i < chunk.closures.len; i++) {
        struct ClosureInfo closure = chunk.closures.ptr[i];
        printf("{ addr: %d, arity: %d, captures: %d }\n", closure.address, closure.arity, closure.capture_count);
    }*/
    //print_instructions(config->output, &chunk);

    run_vm(&chunk, (struct VmConfig){ .out = config->output, .error = config->error });
}

bool compile_file(const struct CompilerConfig config, struct Chunk *out)
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
        return false;
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
        return false;
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

    *out = compiler.chunk;
    return errors.len == 0;
}

bool load_bytecode_file(const struct CompilerConfig config, struct Chunk *out)
{
    FILE *file = fopen(config.file_name, "rb");

    if (file == null) {
        fprintf(config.error, "could not open file '%s'\n", config.file_name);
        return false;
    }

    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    rewind(file);

    char *bytecode = alloc_mem(file_size);
    usize bytes_read = fread(bytecode, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        free_mem(bytecode);
        fclose(file);
        return false;
    }
    fclose(file);

    enum WriteChunkResult result = chunk_from((u8*)bytecode, file_size, out);

    if (result == WRITE_CHUNK_ERR) {
        fprintf(config.error, "error reading bytecode\n");
    }
    free_mem(bytecode);

    return result == WRITE_CHUNK_OK;
}

bool save_bytecode_file(const struct CompilerConfig config, const struct Chunk *chunk)
{
    FILE *file = fopen(config.file_name, "wb");

    if (file == null) {
        fprintf(config.error, "could not open file '%s'\n", config.file_name);
        return false;
    }

    enum WriteChunkResult result = write_chunk_to(file, chunk);

    if (result == WRITE_CHUNK_ERR) {
        fprintf(config.error, "error writing bytecode to file\n");
    }

    fclose(file);

    return result == WRITE_CHUNK_OK;
}

