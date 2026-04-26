#include <stdlib.h>

#include "compiler/compiler.h"

int main(int argc, const char *argv[])
{
	if (argc <= 1)
		return EXIT_SUCCESS;

    const char *file_name = argv[1];
    u32 file_name_len = 0;
    while (file_name[++file_name_len] != '\0') {}

    compile_file((struct CompilerConfig){
        .output = stdout,
        .error = stderr,
        .file_name = file_name,
        .file_name_len = file_name_len,
    });
}
