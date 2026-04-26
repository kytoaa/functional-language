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

struct FunctionBindingNode {
    struct AstNode node;
    const char *src_loc;
    u32 len;
    struct FunctionBindingNode *next_binding;
};

struct DeclarationNode {
    struct AstNode node;
    const char *name;
    u32 name_len;
    bool is_global;
    struct FunctionBindingNode *bindings;
    struct AstNode *body;
    struct DeclarationNode *next_declaration;
};

struct LambdaNode {
    struct AstNode node;
    struct FunctionBindingNode *bindings;
    struct AstNode *body;
};

enum AstBinaryOp {
    AST_BIN_OP_ADD,
    AST_BIN_OP_SUB,
    AST_BIN_OP_MUL,
    AST_BIN_OP_DIV,

    AST_BIN_OP_EQUAL,
    AST_BIN_OP_LESS,
    AST_BIN_OP_LESS_EQ,
    AST_BIN_OP_GREATER,
    AST_BIN_OP_GREATER_EQ,

    AST_BIN_OP_AND,
    AST_BIN_OP_OR,

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
struct UnderscoreNode {
    struct AstNode node;
};

enum LiteralType {
    LITERAL_TYPE_NUMBER,
    LITERAL_TYPE_CHARACTER,
    LITERAL_TYPE_BOOLEAN,
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

struct LetExprNode {
    struct AstNode node;
    struct AstNode *first_decl;
    struct AstNode *body;
};

struct CasePatternNode {
    struct AstNode node;
    struct AstNode *pattern;
    struct AstNode *condition;
    struct AstNode *body;
    struct CasePatternNode *next_pattern;
};

struct CaseExprNode {
    struct AstNode node;
    struct AstNode *value;
    struct AstNode *first_pattern;
};

struct ModuleDeclNode {
    struct AstNode node;
    struct IdentifierNode *name;
    struct DeclarationNode *declarations;
    struct ModuleDeclNode *submodules;
    struct ModuleDeclNode *next_mod;
    bool has_body;
};

struct NamespaceAccessNode {
    struct AstNode node;
    struct IdentifierNode *ident;
    struct AstNode *rhs;
};

struct UseDeclNode {
    struct AstNode node;
    struct AstNode *use_expr;
};

#endif
