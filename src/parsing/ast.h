#ifndef func_lang_parsing_ast_h
#define func_lang_parsing_ast_h

#include "../prelude.h"

#define AST_ALLOC_SIZE 1024

#define ALLOC_NODE(T) (alloc_ast_node(sizeof(T)))

enum NodeKind {
    AST_LITERAL,
    AST_APPLICATION,
    AST_BIN_OP,
    AST_UNARY_OP,
    AST_DECLARATION,
    AST_IDENTIFIER,
    AST_BINDING,
};

struct AstNode {
    enum NodeKind kind;
};

struct AstAllocator {
    struct AstAllocator *next;
    // points to the next free byte
    u8 *top;
    u8 mem[AST_ALLOC_SIZE];
};

struct AstNode *alloc_ast_node(usize size);

#endif
