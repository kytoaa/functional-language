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
		case AST_BIN_OP_CONS:
			printf(" :: ");
			break;
	}
	print_node(node->r);
	printf(")");
}
static void print_identifier(struct IdentifierNode *node)
{
	printf("%.*s", node->len, node->src_loc);
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

static void print_node(struct AstNode *node)
{
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

		case AST_IDENTIFIER:
			print_identifier((struct IdentifierNode*)node);
			break;

        case AST_IF_EXPR:
            print_if_expr((struct IfExprNode*)node);
            break;
	}
}

void print_ast(struct AstNode *root)
{
    print_node(root);
}

