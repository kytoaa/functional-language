#include "reduction.h"
#include "../prelude.h"
#include "../parsing/nodes.h"
#include "../parsing/traversal.h"

static void reduce_node(struct AstNode *node, void *arg);

void reduce_ast(struct AstNode *node)
{
    if (node->kind == AST_DECLARATION) {
        struct DeclarationNode *decl = (struct DeclarationNode*)node;
        while (decl != null) {
            traverse_node(AS_NODE(decl), null, reduce_node, null);
            decl = decl->next_declaration;
        }
    } else {
        traverse_node(node, null, reduce_node, null);
    }
}

/// converts declaration to a lambda
static void reduce_declaration(struct DeclarationNode *decl)
{
    if (decl->bindings == null)
        return;

    struct LambdaNode *lambda = ALLOC_NODE(struct LambdaNode);
    *lambda = (struct LambdaNode){
        .node = { AST_LAMBDA },
        .bindings = decl->bindings,
        .body = decl->body,
    };
    struct FunctionBindingNode *binding = lambda->bindings;
    while (binding != null) {
        binding = binding->next_binding;
    }
    decl->body = AS_NODE(lambda);
    decl->bindings = null;
}

static void reduce_node(struct AstNode *node, void *arg)
{
    if (node == null)
        return;

    if (node->kind == AST_DECLARATION) {
        reduce_declaration((struct DeclarationNode*)node);
    } else if (node->kind == AST_LAMBDA) {
        struct LambdaNode *lambda = (struct LambdaNode*)node;
        struct FunctionBindingNode *binding = lambda->bindings;
        while (binding != null) {
            binding = binding->next_binding;
        }
    }
}

