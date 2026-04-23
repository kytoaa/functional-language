#ifndef func_lang_parsing_ast_h
#define func_lang_parsing_ast_h

#include "../prelude.h"

#define AST_ALLOC_SIZE 1024

#define ALLOC_NODE(T) ((T*)ast_alloc(sizeof(T)))

enum NodeKind {
    AST_LITERAL,
    AST_APPLICATION,
    AST_BIN_OP,
    AST_UNARY_OP,
    AST_DECLARATION,
    AST_IDENTIFIER,
    AST_BINDING,
    AST_IF_EXPR,
    AST_LET_EXPR,
    AST_CASE_EXPR,
    AST_CASE_PATTERN,
    AST_LAMBDA,
    AST_FUNCTION_BINDING,
};

struct Location {
    u32 line;
    u32 file_pos;
};

struct AstNode {
    enum NodeKind kind;
    struct Location loc;
};

struct AstAllocator {
    struct AstAllocator *next;
    // points to the next free byte
    u8 *top;
    u8 mem[AST_ALLOC_SIZE];
};

void *ast_alloc(usize size);

const char *ast_node_name(struct AstNode *node);

void free_ast();

#define AS_NODE(node) ((struct AstNode*)(node))

#endif
