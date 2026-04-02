#ifndef func_lang_parsing_traversal_h
#define func_lang_parsing_traversal_h

#include "ast.h"

void traverse_node(
    struct AstNode *node,
    void *arg,
    void (*pre_callback)(struct AstNode*, void*),
    void (*post_callback)(struct AstNode*, void*)
);

#endif
