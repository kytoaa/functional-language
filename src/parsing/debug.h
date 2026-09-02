#ifndef func_lang_parsing_debug_h
#define func_lang_parsing_debug_h

#include "ast.h"

void print_ast(struct AstTopLevel *top_level);
void print_ast_node(struct AstNode *node);

#endif
