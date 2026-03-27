#ifndef func_lang_parsing_nodes_h
#define func_lang_parsing_nodes_h

#include "ast.h"

static inline bool is_node(struct AstNode *node, enum NodeKind kind)
{
    return node != null && node->kind == kind;
}

struct ApplicationNode {
    struct AstNode node;
    struct AstNode *function;
    struct AstNode *argument;
};

enum AstBinaryOp {
    AST_BIN_OP_ADD,
    AST_BIN_OP_SUB,
    AST_BIN_OP_MUL,
    AST_BIN_OP_DIV,
};

struct BinOpNode {
    struct AstNode node;
    enum AstBinaryOp op;
    struct AstNode *l;
    struct AstNode *r;
};

enum AstUnaryOp {
    AST_UN_OP_NEG,
};

struct UnaryOpNode {
    struct AstNode node;
    enum AstUnaryOp op;
    struct AstNode *val;
};

struct IdentifierNode {
    struct AstNode node;
    const char *src_loc;
    u32 len;
};

#endif
