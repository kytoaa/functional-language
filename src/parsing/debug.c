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
        case LITERAL_TYPE_EMPTY_LIST:
            printf("[]");
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
    if (node->declaration == null) {
        printf("%.*s", node->len, node->src_loc);
    } else {
        printf("[%.*s(%d); ", node->len, node->src_loc, (i8)node->declaration->depth - (i8)node->node.depth );
        switch (node->declaration->kind) {
            case AST_FUNCTION_BINDING:{
                struct FunctionBindingNode *binding = (struct FunctionBindingNode*)node->declaration;
                printf("lambda with [ ");
                print_bindings(((struct LambdaNode*)binding->function)->bindings);
                printf("]]");
                break;
            }
            case AST_DECLARATION:{
                struct DeclarationNode *declaration = (struct DeclarationNode*)node->declaration;
                printf("%.*s]", declaration->name_len, declaration->name);
                break;
            }
            default:{
                panic("unreachable, ident declaration should be decl or lambda");
                break;
            }
        }
    }
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

        case AST_BINDING:
        case AST_CASE_PATTERN:
        case AST_FUNCTION_BINDING:
            printf("error, encountered %s", ast_node_name(node));
            break;
    }
}

void print_ast(struct AstNode *root)
{
    while (root != null) {
        print_node(root);

        if (root->kind == AST_DECLARATION) {
            root = AS_NODE(((struct DeclarationNode*)root)->next_declaration);
        }
        printf("\n");
    }
}

