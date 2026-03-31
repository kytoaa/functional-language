#include "ast.h"
#include "nodes.h"
#include "../lexer.h"

struct Parser {
    struct Token prev;
    struct Token current;
};

static struct Parser parser = {};

static void advance()
{
    parser.prev = parser.current;
    parser.current = next_token();
}

static void consume(enum TokenType type, const char *error_msg)
{
    if (parser.current.type == type) {
        advance();
        return;
    }
    // TODO: error
}

enum Precedence {
    PREC_NONE,
    PREC_EXPR,

    PREC_APPLICATION, // :
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // not -
    PREC_PRIMARY,
};

static struct AstNode *expr(enum Precedence precedence);

static struct AstNode *binary(enum Precedence precedence, struct AstNode *lhs);
static struct AstNode *application(enum Precedence precedence, struct AstNode *lhs);
static struct AstNode *grouping();
static struct AstNode *bracket();
static struct AstNode *unary();

static struct AstNode *identifier();
static struct AstNode *number();
static struct AstNode *boolean();
static struct AstNode *unit();

static struct AstNode *lambda();

static struct AstNode *if_expr();

typedef struct AstNode* (*InfixParseFn)(enum Precedence, struct AstNode*);
typedef struct AstNode* (*PrefixParseFn)();

struct ParseRule {
    PrefixParseFn prefix;
    InfixParseFn infix;
    enum Precedence precedence;
    bool left_associative;
};

static struct ParseRule rules[] = {
    [TOKEN_L_PAREN]      = { grouping, null, PREC_NONE },
    [TOKEN_R_PAREN]      = { null, null, PREC_NONE },

    [TOKEN_L_BRACE]      = { null, null, PREC_NONE },
    [TOKEN_R_BRACE]      = { null, null, PREC_NONE },

    [TOKEN_L_BRACKET]    = { bracket, null, PREC_NONE },
    [TOKEN_R_BRACKET]    = { null, null, PREC_NONE },

    [TOKEN_IDENT]        = { identifier, null, PREC_NONE },

    [TOKEN_ARROW]        = { null, null, PREC_NONE },
    [TOKEN_WIDE_ARROW]   = { null, null, PREC_NONE },

    [TOKEN_FUN]          = { lambda, null, PREC_NONE },

    [TOKEN_LET]          = { null, null, PREC_NONE },
    [TOKEN_IN]           = { null, null, PREC_NONE },

    [TOKEN_IF]           = { if_expr, null, PREC_NONE },
    [TOKEN_THEN]         = { null, null, PREC_NONE },
    [TOKEN_ELSE]         = { null, null, PREC_NONE },

    [TOKEN_ADD]          = { null, binary, PREC_TERM },
    [TOKEN_SUB]          = { unary, binary, PREC_TERM },
    [TOKEN_MUL]          = { null, binary, PREC_FACTOR },
    [TOKEN_DIV]          = { null, binary, PREC_FACTOR },

    [TOKEN_EQUAL]        = { null, null, PREC_COMPARISON },
    [TOKEN_GREATER]      = { null, null, PREC_COMPARISON },
    [TOKEN_GREATER_EQ]   = { null, null, PREC_COMPARISON },
    [TOKEN_LESS]         = { null, null, PREC_COMPARISON },
    [TOKEN_LESS_EQ]      = { null, null, PREC_COMPARISON },

    [TOKEN_COLON]        = { null, application, PREC_APPLICATION },
    [TOKEN_DOUBLE_COLON] = { null, binary, PREC_APPLICATION },

    [TOKEN_AND]          = { null, null, PREC_NONE },
    [TOKEN_OR]           = { null, null, PREC_NONE },
    [TOKEN_NOT]          = { unary, null, PREC_NONE },

    [TOKEN_UNIT]         = { unit, null, PREC_NONE },
    [TOKEN_NUM]          = { number, null, PREC_NONE },
    [TOKEN_CHAR]         = { null, null, PREC_NONE },
    [TOKEN_TRUE]         = { boolean, null, PREC_NONE },
    [TOKEN_FALSE]        = { boolean, null, PREC_NONE },

    [TOKEN_SEMICOLON]    = { null, null, PREC_NONE },
    [TOKEN_EQ]           = { null, null, PREC_NONE },

    [TOKEN_EOF]          = { null, null, PREC_NONE },
    [TOKEN_ERROR]        = { null, null, PREC_NONE },
};

static struct ParseRule *get_rule(enum TokenType type)
{
    return &rules[type];
}

static struct AstNode *grouping()
{
    struct AstNode *node = expr(PREC_EXPR);
    consume(TOKEN_R_PAREN, "expected ')'");
    return node;
}
static struct AstNode *bracket()
{
    if (parser.current.type == TOKEN_R_BRACKET) {
        struct LiteralNode *empty_list = ALLOC_NODE(struct LiteralNode);

        *empty_list = (struct LiteralNode){
            .node = { AST_LITERAL },
            .type = LITERAL_TYPE_EMPTY_LIST,
        };
        advance();

        return (struct AstNode*)empty_list;
    }
    return null;
}
static struct AstNode *unary()
{
    enum TokenType op_type = parser.prev.type;
    enum AstUnaryOp op;
    switch (op_type) {
        case TOKEN_SUB:
            op = AST_UN_OP_NEG;
            break;
        case TOKEN_NOT:
            op = AST_UN_OP_NOT;
            break;
        default:
            // error
            break;
    }

    struct UnaryOpNode *node = ALLOC_NODE(struct UnaryOpNode);
    *node = (struct UnaryOpNode){
        .node = { AST_UNARY_OP },
        .op = op,
        .val = expr(PREC_UNARY),
    };

    return (struct AstNode*)node;
}

static struct AstNode *binary(enum Precedence precedence, struct AstNode *lhs)
{
    enum TokenType op_type = parser.prev.type;
    struct ParseRule *rule = get_rule(op_type);

    enum AstBinaryOp op;
    switch (op_type) {
        case TOKEN_ADD:
            op = AST_BIN_OP_ADD;
            break;
        case TOKEN_SUB:
            op = AST_BIN_OP_SUB;
            break;
        case TOKEN_MUL:
            op = AST_BIN_OP_MUL;
            break;
        case TOKEN_DIV:
            op = AST_BIN_OP_DIV;
            break;
        case TOKEN_DOUBLE_COLON:
            op = AST_BIN_OP_CONS;
            // subtract to make it right associative
            // a :: b :: c = a :: (b :: c)
            precedence -= 1;
            break;
        default:
            // error
            break;
    };

    struct BinOpNode *node = ALLOC_NODE(struct BinOpNode);

    *node = (struct BinOpNode){
        .node = { AST_BIN_OP },
        .op = op,
        .l = lhs,
        .r = expr(precedence + 1),
    };

    return (struct AstNode*)node;
}

static struct AstNode *application(enum Precedence precedence, struct AstNode *lhs)
{
    struct ApplicationNode *node = ALLOC_NODE(struct ApplicationNode);

    *node = (struct ApplicationNode){
        .node = { AST_APPLICATION },
        .function = lhs,
        .argument = expr(PREC_APPLICATION + 1),
    };

    return (struct AstNode*)node;
}

static struct AstNode *identifier()
{
    struct IdentifierNode *ident = ALLOC_NODE(struct IdentifierNode);

    *ident = (struct IdentifierNode){
        .node = { AST_IDENTIFIER },
        .src_loc = parser.prev.start,
        .len = parser.prev.len,
    };

    return (struct AstNode*)ident;
}

static struct AstNode *number()
{
    struct LiteralNode *num = ALLOC_NODE(struct LiteralNode);

    u32 n = 0;
    for (u32 i = 0; i < parser.prev.len; i++) {
        char c = parser.prev.start[i];
        if ('0' <= c && c <= '9') {
            n *= 10;
            n += c - '0';
        } else if (c == '_') {
            continue;
        } else {
            // TODO: error
        }
    }

    *num = (struct LiteralNode){
        .node = { AST_LITERAL },
        .type = LITERAL_TYPE_NUMBER,
        .as.number = n,
    };

    return (struct AstNode*)num;
}

static struct AstNode *boolean()
{
    struct LiteralNode *node = ALLOC_NODE(struct LiteralNode);

    *node = (struct LiteralNode){
        .node = { AST_LITERAL },
        .type = LITERAL_TYPE_BOOLEAN,
        .as.boolean = *parser.prev.start == 't',
    };

    return (struct AstNode*)node;
}

static struct AstNode *unit()
{
    struct LiteralNode *node = ALLOC_NODE(struct LiteralNode);

    *node = (struct LiteralNode){
        .node = { AST_LITERAL },
        .type = LITERAL_TYPE_UNIT,
    };

    return (struct AstNode*)node;
}

static struct AstNode *lambda()
{
    struct LambdaNode *node = ALLOC_NODE(struct LambdaNode);

    struct FunctionBindingNode *binding = null;

    while (parser.current.type != TOKEN_ARROW) {
        advance();
        if (parser.prev.type != TOKEN_IDENT) {
            // error
            return null;
        }
        struct FunctionBindingNode *next_binding = ALLOC_NODE(struct FunctionBindingNode);
        *next_binding = (struct FunctionBindingNode){
            .node = { AST_FUNCTION_BINDING },
            .next_binding = binding,
            .src_loc = parser.prev.start,
            .len = parser.prev.len,
        };
        binding = next_binding;
    }

    if (binding != null) {
        struct FunctionBindingNode *prev = null;
        struct FunctionBindingNode *current = binding;
        struct FunctionBindingNode *next_binding;

        while (current != null) {
            next_binding = current->next_binding;
            current->next_binding = prev;
            prev = current;
            current = next_binding;
        }
        binding = prev;
    }

    consume(TOKEN_ARROW, "expected '->'");

    struct AstNode *body = expr(PREC_EXPR);

    *node = (struct LambdaNode){
        .node = { AST_LAMBDA },
        .bindings = binding,
        .body = body,
    };

    return (struct AstNode*)node;
}

static struct AstNode *if_expr()
{
    struct IfExprNode *node = ALLOC_NODE(struct IfExprNode);

    struct AstNode *condition = expr(PREC_EXPR);
    consume(TOKEN_THEN, "expected 'then'");
    struct AstNode *then_expr = expr(PREC_EXPR);
    consume(TOKEN_ELSE, "expected 'else'");
    struct AstNode *else_expr = expr(PREC_EXPR);

    *node = (struct IfExprNode){
        .node = { AST_IF_EXPR },
        .condition = condition,
        .then_expr = then_expr,
        .else_expr = else_expr,
    };

    return (struct AstNode*)node;
}

static struct AstNode *expr(enum Precedence precedence)
{
    advance();
    PrefixParseFn prefix = get_rule(parser.prev.type)->prefix;
    if (prefix == null) {
        return null;
    }

    struct AstNode *lhs = prefix();

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        struct ParseRule *infix = get_rule(parser.prev.type);

        lhs = infix->infix(infix->precedence, lhs);
    }

    return lhs;
}

struct AstNode *build_ast(const char *src)
{
    init_lexer(src);

    advance();

    return expr(PREC_EXPR);
}

