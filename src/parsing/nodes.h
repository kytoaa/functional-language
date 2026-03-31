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

    AST_BIN_OP_CONS,
};

struct BinOpNode {
    struct AstNode node;
    enum AstBinaryOp op;
    struct AstNode *l;
    struct AstNode *r;
};

enum AstUnaryOp {
    AST_UN_OP_NEG,
    AST_UN_OP_NOT,
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

enum LiteralType {
    LITERAL_TYPE_NUMBER,
    LITERAL_TYPE_CHARACTER,
    LITERAL_TYPE_BOOLEAN,
    LITERAL_TYPE_EMPTY_LIST,
    LITERAL_TYPE_UNIT,
};

struct LiteralNode {
    struct AstNode node;
    union {
        u32 number;
        u32 character;
        bool boolean;
    } as;
    enum LiteralType type;
};

struct IfExprNode {
    struct AstNode node;
    struct AstNode *condition;
    struct AstNode *then_expr;
    struct AstNode *else_expr;
};
struct FunctionBindingNode {
    struct AstNode node;
    const char *src_loc;
    u32 len;
    struct FunctionBindingNode *next_binding;
};

struct LambdaNode {
    struct AstNode node;
    struct FunctionBindingNode *bindings;
    struct AstNode *body;
};

#endif
