#include "reduction.h"
#include "../prelude.h"
#include "../parsing/nodes.h"
#include "../parsing/traversal.h"

static void reduce_node(struct AstNode *node, void *arg);

void reduce_ast(struct AstTopLevel *ast)
{
    struct DeclarationNode *decl = (struct DeclarationNode*)ast->declarations;
    while (decl != null) {
        traverse_node(AS_NODE(decl), null, reduce_node, null);
        decl = decl->next_declaration;
    }
    struct ModuleDeclNode *mod = (struct ModuleDeclNode*)ast->modules;
    while (mod != null) {
        traverse_node(AS_NODE(mod), null, reduce_node, null);
        mod = mod->next_mod;
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

