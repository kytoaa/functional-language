#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parsing/debug.h"
#include "parsing/parser.h"

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

    struct AstNode *node = build_ast(argv[1]);
    print_ast(node);
    printf("\n");
}
