#include <stdlib.h>

#include "bytecode.h"
#include "compiler/compiler.h"
#include "args.h"
#include "vm/vm.h"

int main(int argc, char *const argv[])
{
	if (argc <= 1)
		return EXIT_FAILURE;

    struct Args args = {};
    if (!parse_args(argc, argv, &args)) {
        return EXIT_FAILURE;
    }

    struct Chunk chunk = {};
    bool compiled = false;

    if (args.file_name != null) {
        u32 file_name_len = 0;
        while (args.file_name[++file_name_len] != '\0') {}

        compiled = compile_file((struct CompilerConfig){
            .output = stdout,
            .error = stderr,
            .file_name = args.file_name,
            .file_name_len = file_name_len,
        }, &chunk);
    } else if (args.bin_output != null) {
        u32 file_name_len = 0;
        while (args.bin_output[++file_name_len] != '\0') {}

        compiled = load_bytecode_file((struct CompilerConfig){
            .output = stdout,
            .error = stderr,
            .file_name = args.bin_output,
            .file_name_len = file_name_len,
        }, &chunk);
    }

    if (compiled) {
        if (args.file_name != null && args.bin_output != null) {
            u32 file_name_len = 0;
            while (args.bin_output[++file_name_len] != '\0') {}

            save_bytecode_file((struct CompilerConfig){
                .output = stdout,
                .error = stderr,
                .file_name = args.bin_output,
                .file_name_len = file_name_len,
            }, &chunk);
        } else {
            run_vm(&chunk, (struct VmConfig){ .out = stdout, .error = stderr });
        }

        free_chunk(&chunk);
        return EXIT_SUCCESS;
    } else {
        free_chunk(&chunk);
        return EXIT_FAILURE;
    }
}
