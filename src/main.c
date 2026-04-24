#include <stdio.h>
#include <stdlib.h>

#include "compiler/compiler.h"
#include "lexer.h"

int main(int argc, const char *argv[])
{
	if (argc <= 1)
		return EXIT_SUCCESS;

    FILE *file = fopen(argv[1], "rb");
    if (file == null) {
        fprintf(stderr, "could not open file \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    rewind(file);

    char *buffer = alloc_mem(file_size + 1);
    usize bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "failed to read entire file\n");
        exit(EXIT_FAILURE);
    }
    buffer[bytes_read] = '\0';
    fclose(file);

	init_lexer(buffer);

    compile_file(&(struct CompilerConfig){
        .output = stdout,
        .error = stderr,
        .src = buffer,
        .file_name = argv[1],
        .file_name_len = 4,
    });
}
