#include <stdio.h>
#include <stdlib.h>

#include "compiler/compiler.h"
#include "lexer.h"
#include "parsing/debug.h"
#include "parsing/parser.h"
#include "parsing/traversal.h"

static void print_node_name(struct AstNode *node, void *arg)
{
    printf("%s ", ast_node_name(node));
}

int main(int argc, const char *argv[])
{
	if (argc <= 1)
		return EXIT_SUCCESS;

	init_lexer(argv[1]);

	struct Token current;
	for (;;) {
		current = next_token();

		if (current.type == TOKEN_ERROR) {
			printf("error at line %d: %s\n", current.line, current.start);
			exit(EXIT_FAILURE);
		}

		printf("%s @ %d, %d :: %.*s\n", token_type_name(current.type), current.line, current.len, current.len, current.start);
		if (current.type == TOKEN_EOF) {
            break;
		}
	}

    compile_file(&(struct CompilerConfig){
        .output = stdout,
        .error = stderr,
        .src = argv[1],
        .file_name = "repl",
        .file_name_len = 4,
    });
}
