#include "ast.h"
#include "nodes.h"
#include <stdio.h>

static void print_node(struct AstNode *node);

static void print_literal(struct LiteralNode *node)
{
    switch (node->type) {
        case LITERAL_TYPE_BOOLEAN:
            printf("%s", node->as.boolean ? "true" : "false");
            break;
        case LITERAL_TYPE_CHARACTER:
            printf("%c", node->as.character);
            break;
        case LITERAL_TYPE_NUMBER:
            printf("%d", node->as.number);
            break;
        case LITERAL_TYPE_UNIT:
            printf("()");
            break;
    }
}
static void print_application(struct ApplicationNode *node)
{
    printf("(");
    print_node(node->function);
    printf(" : ");
    print_node(node->argument);
    printf(")");
}
static void print_binary_op(struct BinOpNode *node)
{
    printf("(");
    print_node(node->l);
    switch (node->op) {
        case AST_BIN_OP_ADD:
            printf(" + ");
            break;
        case AST_BIN_OP_SUB:
            printf(" - ");
            break;
        case AST_BIN_OP_MUL:
            printf(" * ");
            break;
        case AST_BIN_OP_DIV:
            printf(" / ");
            break;
        case AST_BIN_OP_EQUAL:
            printf(" == ");
            break;
        case AST_BIN_OP_LESS:
            printf(" < ");
            break;
        case AST_BIN_OP_LESS_EQ:
            printf(" <= ");
            break;
        case AST_BIN_OP_GREATER:
            printf(" > ");
            break;
        case AST_BIN_OP_GREATER_EQ:
            printf(" >= ");
            break;
        case AST_BIN_OP_AND:
            printf(" and ");
            break;
        case AST_BIN_OP_OR:
            printf(" or ");
            break;
        case AST_BIN_OP_CONS:
            printf(" :: ");
            break;
    }
    print_node(node->r);
    printf(")");
}
static void print_unary_op(struct UnaryOpNode *node)
{
    printf("(");
    switch (node->op) {
        case AST_UN_OP_NEG:
            printf("-");
            break;
        case AST_UN_OP_NOT:
            printf("not ");
            break;
        case AST_UN_OP_FORCE:
            printf("$");
            break;
    }
    print_node(node->val);
    printf(")");
}

static void print_bindings(struct FunctionBindingNode *node)
{
    while (node != null) {
        printf("%.*s ", node->len, node->src_loc);
        node = node->next_binding;
    }
}

static void print_identifier(struct IdentifierNode *node)
{
    printf("%.*s", node->len, node->src_loc);
}

static void print_attribute(struct AttributeNode *node)
{
    printf("@");
    print_identifier(node->ident);
    if (node->body != null) {
        printf("(");
        print_node(node->body);
        printf(")");
    }
}

static void print_namespace_access(struct NamespaceAccessNode *node)
{
    print_identifier(node->ident);
    printf("..");
    print_node(node->rhs);
}

static void print_lambda(struct LambdaNode *node)
{
    printf("(fun ");

    print_bindings(node->bindings);

    printf("-> ");
    print_node(node->body);
    printf(")");
}
static void print_declaration(struct DeclarationNode *node)
{
    printf("%.*s ", node->name_len, node->name);
    print_bindings(node->bindings);

    printf("= ");
    print_node(node->body);
    printf(";\n");
}

static void print_if_expr(struct IfExprNode *node)
{
    printf("(if ");
    print_node(node->condition);
    printf(" then ");
    print_node(node->then_expr);
    printf(" else ");
    print_node(node->else_expr);
    printf(")");
}
static void print_let_expr(struct LetExprNode *node)
{
    printf("let\n");
    struct DeclarationNode *decl = (struct DeclarationNode*)node->first_decl;
    while (decl != null) {
        printf("\t");
        print_declaration(decl);
        decl = decl->next_declaration;
    }
    printf("\t in ");
    print_node(node->body);
}
static void print_case_expr(struct CaseExprNode *node)
{
    printf("(case ");
    print_node(node->value);
    printf(" of");

    struct CasePatternNode *pat = (struct CasePatternNode*)node->first_pattern;
    while (pat != null) {
        printf("\n\t| ");
        print_node(pat->pattern);
        if (pat->condition != null) {
            printf(" if ");
            print_node(pat->condition);
        }
        printf(" -> ");
        print_node(pat->body);
        pat = pat->next_pattern;
    }
    printf(")");
}

static void print_node(struct AstNode *node)
{
    if (node == null)
        return;

    switch (node->kind) {
        case AST_LITERAL:
            print_literal((struct LiteralNode*)node);
            break;
        case AST_APPLICATION:
            print_application((struct ApplicationNode*)node);
            break;
        case AST_BIN_OP:
            print_binary_op((struct BinOpNode*)node);
            break;
        case AST_UNARY_OP:
            print_unary_op((struct UnaryOpNode*)node);
            break;

        case AST_IDENTIFIER:
            print_identifier((struct IdentifierNode*)node);
            break;
        case AST_UNDERSCORE:
            printf("_");
            break;
        case AST_NAMESPACE_ACCESS:
            print_namespace_access((struct NamespaceAccessNode*)node);
            break;

        case AST_IF_EXPR:
            print_if_expr((struct IfExprNode*)node);
            break;
        case AST_LET_EXPR:
            print_let_expr((struct LetExprNode*)node);
            break;
        case AST_CASE_EXPR:
            print_case_expr((struct CaseExprNode*)node);
            break;

        case AST_LAMBDA:
            print_lambda((struct LambdaNode*)node);
            break;

        case AST_DECLARATION:
            print_declaration((struct DeclarationNode*)node);
            break;

        case AST_ATTR:
            print_attribute((struct AttributeNode*)node);
            break;

        case AST_CONSTRUCTOR:
            printf("<constructor>");
            break;

        case AST_BINDING:
        case AST_CASE_PATTERN:
        case AST_FUNCTION_BINDING:
            printf("error, encountered %s", ast_node_name(node));
            break;
        case AST_MODULE_DECL:
        case AST_USE_DECL:
            break;
    }
}

void print_ast(struct AstTopLevel *top_level)
{
    if (top_level == null)
        return;
    struct AstNode *decl = top_level->declarations;
    while (decl != null) {
        print_node(decl);

        if (decl->kind == AST_DECLARATION) {
            decl = AS_NODE(((struct DeclarationNode*)decl)->next_declaration);
        }
        printf("\n");
    }
}

