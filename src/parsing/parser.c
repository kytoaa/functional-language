#include "parser.h"
#include "ast.h"
#include "nodes.h"
#include "../lexer.h"

struct Parser {
    struct Token prev;
    struct Token current;
    bool has_error;
    struct ParseError err;
    const char *src;
};

static struct Parser parser = {};

static void advance()
{
    parser.prev = parser.current;
    parser.current = next_token();
}
static void error(struct Token token, const char *msg)
{
    if (parser.has_error)
        return;

    parser.has_error = true;
    parser.err.token = token;
    parser.err.msg = msg;
}

static inline struct Location prev_loc()
{
    return (struct Location){
        .line = parser.prev.line,
        .file_pos = parser.prev.start - parser.src,
    };
}

static void consume(enum TokenType type, const char *error_msg)
{
    if (parser.current.type == type) {
        advance();
        return;
    }
    error(parser.current.type == TOKEN_EOF ? parser.prev : parser.current, error_msg);
}

static void *reverse_linked_list(void *first, void **(*get_next)(void*))
{
    if (first != null) {
        void *prev = null;
        void *current = first;
        void *next;

        while (current != null) {
            next = *get_next(current);
            *get_next(current) = prev;
            prev = current;
            current = next;
        }
        return prev;
    }
    return null;
}

enum Precedence {
    PREC_NONE,
    PREC_EXPR,

    PREC_COLON, // :
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // not -
    PREC_APPLICATION, // a b
    PREC_NAMESPACE,
    PREC_PRIMARY,
};

static struct AstNode *declaration();
static struct AstNode *expr(enum Precedence precedence);

static struct AstNode *module();

static struct AstNode *binary(enum Precedence precedence, struct AstNode *lhs);
static struct AstNode *application(enum Precedence precedence, struct AstNode *lhs);
static struct AstNode *namespace(enum Precedence precedence, struct AstNode *lhs);
static struct AstNode *grouping();
static struct AstNode *unary();

static struct AstNode *identifier();
static struct AstNode *underscore();
static struct AstNode *number();
static struct AstNode *boolean();
static struct AstNode *character();
static struct AstNode *unit();

static struct AstNode *lambda();

static struct AstNode *if_expr();
static struct AstNode *let_expr();
static struct AstNode *case_expr();

static struct AstNode *error_token()
{
    error(parser.prev, "encountered error token");
    return null;
}

typedef struct AstNode* (*InfixParseFn)(enum Precedence, struct AstNode*);
typedef struct AstNode* (*PrefixParseFn)();

struct ParseRule {
    PrefixParseFn prefix;
    InfixParseFn infix;
    enum Precedence infix_precedence;
    bool leave_op_token;
};

static struct ParseRule rules[] = {
    [TOKEN_L_PAREN]      = { grouping, application, PREC_APPLICATION, true },
    [TOKEN_R_PAREN]      = { null, null, PREC_NONE },

    [TOKEN_L_BRACE]      = { null, null, PREC_NONE },
    [TOKEN_R_BRACE]      = { null, null, PREC_NONE },

    [TOKEN_L_BRACKET]    = { null, null, PREC_NONE },
    [TOKEN_R_BRACKET]    = { null, null, PREC_NONE },

    [TOKEN_IDENT]        = { identifier, application, PREC_APPLICATION, true },
    [TOKEN_UNDERSCORE]   = { underscore, null, PREC_NONE },

    [TOKEN_ARROW]        = { null, null, PREC_NONE },
    [TOKEN_WIDE_ARROW]   = { null, null, PREC_NONE },

    [TOKEN_TWO_DOT]      = { null, namespace, PREC_NAMESPACE },

    [TOKEN_FUN]          = { lambda, null, PREC_NONE },

    [TOKEN_LET]          = { let_expr, null, PREC_NONE },
    [TOKEN_IN]           = { null, null, PREC_NONE },

    [TOKEN_IF]           = { if_expr, null, PREC_NONE },
    [TOKEN_THEN]         = { null, null, PREC_NONE },
    [TOKEN_ELSE]         = { null, null, PREC_NONE },
    
    [TOKEN_CASE]         = { case_expr, null, PREC_NONE },
    [TOKEN_OF]           = { null, null, PREC_NONE },

    [TOKEN_MOD]          = { null, null, PREC_NONE },
    [TOKEN_USE]          = { null, null, PREC_NONE },

    [TOKEN_ADD]          = { null, binary, PREC_TERM },
    [TOKEN_SUB]          = { unary, binary, PREC_TERM },
    [TOKEN_MUL]          = { null, binary, PREC_FACTOR },
    [TOKEN_DIV]          = { null, binary, PREC_FACTOR },
    [TOKEN_FORCE]        = { unary, null, PREC_APPLICATION },

    [TOKEN_EQUAL]        = { null, binary, PREC_COMPARISON },
    [TOKEN_GREATER]      = { null, binary, PREC_COMPARISON },
    [TOKEN_GREATER_EQ]   = { null, binary, PREC_COMPARISON },
    [TOKEN_LESS]         = { null, binary, PREC_COMPARISON },
    [TOKEN_LESS_EQ]      = { null, binary, PREC_COMPARISON },

    [TOKEN_COLON]        = { null, application, PREC_COLON },
    [TOKEN_DOUBLE_COLON] = { null, binary, PREC_APPLICATION },

    [TOKEN_AND]          = { null, binary, PREC_AND },
    [TOKEN_OR]           = { null, binary, PREC_OR },
    [TOKEN_NOT]          = { unary, null, PREC_UNARY },

    [TOKEN_UNIT]         = { unit, application, PREC_APPLICATION, true },
    [TOKEN_NUM]          = { number, application, PREC_APPLICATION, true },
    [TOKEN_CHAR]         = { character, application, PREC_APPLICATION, true },
    [TOKEN_TRUE]         = { boolean, application, PREC_APPLICATION, true },
    [TOKEN_FALSE]        = { boolean, application, PREC_APPLICATION, true },

    [TOKEN_SEMICOLON]    = { null, null, PREC_NONE },
    [TOKEN_PIPE]         = { null, null, PREC_NONE },
    [TOKEN_EQ]           = { null, null, PREC_NONE },

    [TOKEN_EOF]          = { null, null, PREC_NONE },
    [TOKEN_ERROR]        = { error_token, null, PREC_NONE },
};

static struct ParseRule *get_rule(enum TokenType type)
{
    return &rules[type];
}

static struct AstNode *grouping()
{
    struct AstNode *node = expr(PREC_EXPR);
    consume(TOKEN_R_PAREN, "expected `)`");
    return node;
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
        case TOKEN_FORCE:
            op = AST_UN_OP_FORCE;
            break;
        default:
            error(parser.prev, "unreachable");
            return null;
    }

    struct UnaryOpNode *node = ALLOC_NODE(struct UnaryOpNode);
    *node = (struct UnaryOpNode){
        .node = { AST_UNARY_OP, prev_loc() },
        .op = op,
        .val = expr(PREC_UNARY),
    };

    return AS_NODE(node);
}

static struct AstNode *binary(enum Precedence precedence, struct AstNode *lhs)
{
    enum TokenType op_type = parser.prev.type;

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
        case TOKEN_EQUAL:
            op = AST_BIN_OP_EQUAL;
            break;
        case TOKEN_LESS:
            op = AST_BIN_OP_LESS;
            break;
        case TOKEN_LESS_EQ:
            op = AST_BIN_OP_LESS_EQ;
            break;
        case TOKEN_GREATER:
            op = AST_BIN_OP_GREATER;
            break;
        case TOKEN_GREATER_EQ:
            op = AST_BIN_OP_GREATER_EQ;
            break;
        case TOKEN_AND:
            op = AST_BIN_OP_AND;
            break;
        case TOKEN_OR:
            op = AST_BIN_OP_OR;
            break;
        default:
            error(parser.prev, "unreachable: invalid binary op");
            return null;
    };

    struct BinOpNode *node = ALLOC_NODE(struct BinOpNode);

    struct Location loc = prev_loc();
    *node = (struct BinOpNode){
        .node = { AST_BIN_OP, loc },
        .op = op,
        .l = lhs,
        .r = expr(precedence + 1),
    };

    return AS_NODE(node);
}

static struct AstNode *application(enum Precedence precedence, struct AstNode *lhs)
{
    struct ApplicationNode *node = ALLOC_NODE(struct ApplicationNode);

    struct Location loc = prev_loc();
    *node = (struct ApplicationNode){
        .node = { AST_APPLICATION, loc },
        .function = lhs,
        .argument = expr(precedence + 1),
    };

    return AS_NODE(node);
}
static struct AstNode *namespace(enum Precedence precedence, struct AstNode *lhs)
{
    if (lhs->kind != AST_IDENTIFIER) {
        error(parser.prev, "namespace lhs must be an identifier");
        return null;
    }
    struct NamespaceAccessNode *node = ALLOC_NODE(struct NamespaceAccessNode);

    struct Location loc = prev_loc();

    struct AstNode *rhs = expr(PREC_NAMESPACE);
    if (rhs->kind != AST_IDENTIFIER && rhs->kind != AST_NAMESPACE_ACCESS) {
        error(parser.prev, "namespace rhs must be an identifier or namespace access");
        return null;
    }

    *node = (struct NamespaceAccessNode){
        .node = { AST_NAMESPACE_ACCESS, loc },
        .ident = (struct IdentifierNode*)lhs,
        .rhs = rhs,
    };

    return AS_NODE(node);
}

static struct AstNode *identifier()
{
    struct IdentifierNode *ident = ALLOC_NODE(struct IdentifierNode);

    *ident = (struct IdentifierNode){
        .node = { AST_IDENTIFIER, prev_loc() },
        .src_loc = parser.prev.start,
        .len = parser.prev.len,
    };

    return AS_NODE(ident);
}
static struct AstNode *underscore()
{
    struct UnderscoreNode *underscore = ALLOC_NODE(struct UnderscoreNode);

    *underscore = (struct UnderscoreNode){
        .node = { AST_UNDERSCORE, prev_loc() },
    };

    return AS_NODE(underscore);
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
            error(parser.prev, "unreachable: invalid number");
            return null;
        }
    }

    *num = (struct LiteralNode){
        .node = { AST_LITERAL, prev_loc() },
        .type = LITERAL_TYPE_NUMBER,
        .as.number = n,
    };

    return AS_NODE(num);
}

static struct AstNode *character()
{
    struct LiteralNode *node = ALLOC_NODE(struct LiteralNode);
    u32 character = parser.prev.start[1];
    if (character == '\\') {
        #define char_case(a, b) case a: character = b; break;
        switch (parser.prev.start[2]) {
            char_case('n', '\n')
            char_case('t', '\t')
            char_case('r', '\r')
            char_case('v', '\v')
            char_case('0', '\0')
            char_case('\'', '\'')
            char_case('\\', '\\')
        }
        #undef char_case
    }

    *node = (struct LiteralNode){
        .node = { AST_LITERAL, prev_loc() },
        .type = LITERAL_TYPE_CHARACTER,
        .as.character = character,
    };

    return AS_NODE(node);
}

static struct AstNode *boolean()
{
    struct LiteralNode *node = ALLOC_NODE(struct LiteralNode);

    *node = (struct LiteralNode){
        .node = { AST_LITERAL, prev_loc() },
        .type = LITERAL_TYPE_BOOLEAN,
        .as.boolean = *parser.prev.start == 't',
    };

    return AS_NODE(node);
}

static struct AstNode *unit()
{
    struct LiteralNode *node = ALLOC_NODE(struct LiteralNode);

    *node = (struct LiteralNode){
        .node = { AST_LITERAL, prev_loc() },
        .type = LITERAL_TYPE_UNIT,
    };

    return AS_NODE(node);
}

static void **binding_get_next(void *binding)
{
    return (void*)&((struct FunctionBindingNode*)binding)->next_binding;
}

static struct FunctionBindingNode *bindings(enum TokenType end)
{
    struct FunctionBindingNode *binding = null;

    while (parser.current.type != end) {
        advance();
        if (parser.prev.type != TOKEN_IDENT) {
            error(parser.prev, "bindings must be identifiers");
            return null;
        }
        struct FunctionBindingNode *next_binding = ALLOC_NODE(struct FunctionBindingNode);
        *next_binding = (struct FunctionBindingNode){
            .node = { AST_FUNCTION_BINDING, prev_loc() },
            .next_binding = binding,
            .src_loc = parser.prev.start,
            .len = parser.prev.len,
        };
        binding = next_binding;
    }

    binding = reverse_linked_list(binding, binding_get_next);
    return binding;
}

static struct AstNode *lambda()
{
    struct LambdaNode *node = ALLOC_NODE(struct LambdaNode);
    struct Location loc = prev_loc();

    struct FunctionBindingNode *binding = bindings(TOKEN_ARROW);

    consume(TOKEN_ARROW, "expected `->`");

    struct AstNode *body = expr(PREC_EXPR);

    *node = (struct LambdaNode){
        .node = { AST_LAMBDA, loc },
        .bindings = binding,
        .body = body,
    };

    return AS_NODE(node);
}

static struct AstNode *if_expr()
{
    struct IfExprNode *node = ALLOC_NODE(struct IfExprNode);
    struct Location loc = prev_loc();

    struct AstNode *condition = expr(PREC_EXPR);
    consume(TOKEN_THEN, "expected `then`");
    struct AstNode *then_expr = expr(PREC_EXPR);
    consume(TOKEN_ELSE, "expected `else`");
    struct AstNode *else_expr = expr(PREC_EXPR);

    *node = (struct IfExprNode){
        .node = { AST_IF_EXPR, loc },
        .condition = condition,
        .then_expr = then_expr,
        .else_expr = else_expr,
    };

    return AS_NODE(node);
}

static void **declaration_get_next(void *decl)
{
    return (void*)&((struct DeclarationNode*)decl)->next_declaration;
}

static struct AstNode *let_expr()
{
    struct LetExprNode *node = ALLOC_NODE(struct LetExprNode);
    struct Location loc = prev_loc();

    struct DeclarationNode *current_decl = null;

    while (parser.current.type != TOKEN_IN) {
        struct DeclarationNode *decl = (struct DeclarationNode*)declaration();
        if (decl != null) {
            decl->next_declaration = current_decl;
        }
        if (parser.has_error)
            return null;
        current_decl = decl;
    }

    current_decl = reverse_linked_list(current_decl, declaration_get_next);
    consume(TOKEN_IN, "unreachable: expected `in`");

    struct AstNode *body = expr(PREC_EXPR);

    *node = (struct LetExprNode){
        .node = { AST_LET_EXPR, loc },
        .first_decl = AS_NODE(current_decl),
        .body = body,
    };

    return AS_NODE(node);
}

static void check_valid_pattern(struct AstNode *pat)
{
    bool searching = true;
    while (searching) {
        if (pat == null)
            return;
        switch (pat->kind) {
            case AST_BIN_OP:{
                struct BinOpNode *bin_op = (struct BinOpNode*)pat;
                if (bin_op->op != AST_BIN_OP_CONS) {
                    error(parser.prev, "cons (`::`) is the only valid binary op pattern");
                    searching = false;
                    break;
                }
                check_valid_pattern(bin_op->l);
                pat = bin_op->r;
                if (pat == null) {
                    searching = false;
                }
                break;
            }
            case AST_LITERAL:
            case AST_IDENTIFIER:
            case AST_UNDERSCORE:
                searching = false;
                break;
            default:
                error(parser.prev, "invalid pattern");
                searching = false;
                break;
        }
    }
}

static void **case_pattern_get_next(void *pat)
{
    return (void*)&((struct CasePatternNode*)pat)->next_pattern;
}

static struct AstNode *case_expr()
{
    struct CaseExprNode *node = ALLOC_NODE(struct CaseExprNode);
    struct Location loc = prev_loc();

    struct AstNode *condition = expr(PREC_EXPR);
    consume(TOKEN_OF, "expected `of` after expression in `case`");

    struct CasePatternNode *pattern = null;

    for (;;) {
        if (parser.current.type != TOKEN_PIPE)
            break;
        advance();
        struct Location pat_loc = prev_loc();

        struct AstNode *pat_expr = expr(PREC_EXPR);

        check_valid_pattern(pat_expr);

        struct AstNode *pat_cond = null;

        if (parser.current.type == TOKEN_IF) {
            advance();
            pat_cond = expr(PREC_EXPR);
        }
        consume(TOKEN_ARROW, "expected `->` after pattern");

        struct AstNode *pat_body = expr(PREC_EXPR);

        struct CasePatternNode *current_pattern = ALLOC_NODE(struct CasePatternNode);

        *current_pattern = (struct CasePatternNode){
            .node = { AST_CASE_PATTERN, pat_loc },
            .pattern = pat_expr,
            .condition = pat_cond,
            .body = pat_body,
            .next_pattern = pattern,
        };
        pattern = current_pattern;
    }
    pattern = reverse_linked_list(pattern, case_pattern_get_next);

    *node = (struct CaseExprNode){
        .node = { AST_CASE_EXPR, loc },
        .value = condition,
        .first_pattern = AS_NODE(pattern),
    };

    return AS_NODE(node);
}

static struct AstNode *expr(enum Precedence precedence)
{
    advance();
    PrefixParseFn prefix = get_rule(parser.prev.type)->prefix;
    if (prefix == null) {
        error(parser.prev, "unexpected token in prefix expression");
        return null;
    }

    struct AstNode *lhs = prefix();

    while (precedence <= get_rule(parser.current.type)->infix_precedence) {
        struct ParseRule *infix = null;
        if (get_rule(parser.current.type)->leave_op_token) {
            infix = get_rule(parser.current.type);
        } else {
            advance();
            infix = get_rule(parser.prev.type);
        }

        if (infix == null || infix->infix == null) {
            error(parser.prev, "unexpected token in infix expression");
            return null;
        }

        lhs = infix->infix(infix->infix_precedence, lhs);
    }

    return lhs;
}

static struct AstNode *declaration()
{
    if (parser.current.type != TOKEN_IDENT) {
        error(parser.prev, "expected function declaration");
        return null;
    }
    advance();

    const char *name = parser.prev.start;
    u32 name_len = parser.prev.len;

    struct DeclarationNode *declaration = ALLOC_NODE(struct DeclarationNode);
    struct Location loc = prev_loc();

    struct FunctionBindingNode *binding = bindings(TOKEN_EQ);

    consume(TOKEN_EQ, "expected `=`");

    *declaration = (struct DeclarationNode){
        .node = { AST_DECLARATION, loc },
        .name = name,
        .name_len = name_len,
        .is_global = false,
        .bindings = binding,
        .body = expr(PREC_EXPR),
    };

    consume(TOKEN_SEMICOLON, "expected `;` after declaration");

    return AS_NODE(declaration);
}

static void **module_get_next(void *mod)
{
    return (void*)&((struct ModuleDeclNode*)mod)->next_mod;
}
static struct AstNode *module()
{
    struct Location loc = prev_loc();
    advance();

    struct IdentifierNode *ident = null;
    if (parser.current.type == TOKEN_IDENT) {
        advance();
        ident = (struct IdentifierNode*)identifier();
    }
    struct DeclarationNode *current_decl = null;
    struct ModuleDeclNode *current_submodule = null;
    bool has_body = false;
    if (parser.current.type == TOKEN_EQ) {
        advance();
        if (parser.current.type == TOKEN_IDENT) {
            struct AstNode *path = expr(PREC_NAMESPACE);
            if (path->kind != AST_NAMESPACE_ACCESS && path->kind != AST_IDENTIFIER) {
                error(parser.prev, "expected a module path");
                return null;
            }
            current_decl = (struct DeclarationNode*)path;
        } else {
            has_body = true;
            consume(TOKEN_L_BRACE, "expected `{` after module");

            while (parser.current.type != TOKEN_R_BRACE) {
                if (parser.current.type == TOKEN_MOD) {
                    struct ModuleDeclNode *submodule = (struct ModuleDeclNode*)module();
                    if (submodule != null) {
                        submodule->next_mod = current_submodule;
                    }
                    if (parser.has_error)
                        return null;
                    current_submodule = submodule;
                } else {
                    struct DeclarationNode *decl = (struct DeclarationNode*)declaration();
                    if (decl != null) {
                        decl->next_declaration = current_decl;
                    }
                    if (parser.has_error)
                        return null;
                    current_decl = decl;
                }
            }

            current_decl = reverse_linked_list(current_decl, declaration_get_next);
            current_submodule = reverse_linked_list(current_submodule, module_get_next);
            consume(TOKEN_R_BRACE, "unreachable: expected `}`");
        }
    }
    if (ident == null && current_decl == null) {
        error(parser.prev, "modules must have a name, body or both");
        return null;
    }
    consume(TOKEN_SEMICOLON, "expected `;` after module declaration");

    struct ModuleDeclNode *node = ALLOC_NODE(struct ModuleDeclNode);

    *node = (struct ModuleDeclNode){
        .node = { AST_MODULE_DECL, loc },
        .name = ident,
        .declarations = current_decl,
        .submodules = current_submodule,
        .next_mod = null,
        .has_body = has_body,
    };

    return AS_NODE(node);
}

bool build_ast(const char *src, struct AstTopLevel *out, struct ParseError *err)
{
    init_lexer(src);
    parser.src = src;

    advance();

    struct ModuleDeclNode *modules = null;
    struct DeclarationNode *declarations = null;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.current.type == TOKEN_MOD) {
            struct ModuleDeclNode *current = (struct ModuleDeclNode*)module();

            if (parser.has_error) {
                *err = parser.err;
                return false;
            }

            current->next_mod = modules;
            modules = current;
        } else {
            struct DeclarationNode *current = (struct DeclarationNode*)declaration();

            if (parser.has_error) {
                *err = parser.err;
                return false;
            }
            current->is_global = true;

            current->next_declaration = declarations;
            declarations = current;
        }
    }
    declarations = reverse_linked_list(declarations, declaration_get_next);
    modules = reverse_linked_list(modules, module_get_next);

    *out = (struct AstTopLevel){
        .declarations = AS_NODE(declarations),
        .modules = AS_NODE(modules),
    };

    return true;
}

