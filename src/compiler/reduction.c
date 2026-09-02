#include "reduction.h"
#include "../prelude.h"
#include "../parsing/nodes.h"
#include "../parsing/traversal.h"

static void reduce_node(struct AstNode *node, void *arg);
static void post_reduce_node(struct AstNode *node, void *arg);
static void reduce_use_decls(struct UseExprNode *use_decl, struct DeclarationNode **decls);

void reduce_ast(struct AstTopLevel *ast)
{
    reduce_use_decls((struct UseExprNode*)ast->use_declarations, (struct DeclarationNode**)&ast->declarations);

    struct UseExprNode *use = (struct UseExprNode*)ast->use_declarations;
    while (use != null) {
        traverse_node(AS_NODE(use), null, reduce_node, post_reduce_node);
        use = use->next_use;
    }

    struct DeclarationNode *decl = (struct DeclarationNode*)ast->declarations;
    while (decl != null) {
        traverse_node(AS_NODE(decl), null, reduce_node, post_reduce_node);
        decl = decl->next_declaration;
    }

    struct ModuleDeclNode *mod = (struct ModuleDeclNode*)ast->modules;
    while (mod != null) {
        traverse_node(AS_NODE(mod), null, reduce_node, post_reduce_node);
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

struct NamespaceClone {
    struct NamespaceAccessNode *root;
    struct IdentifierNode **last;
};

static struct NamespaceClone clone_namespace(struct NamespaceAccessNode *namespace)
{
    struct NamespaceAccessNode *clone = ALLOC_NODE(struct NamespaceAccessNode);
    *clone = *namespace;

    if (clone->rhs->kind == AST_IDENTIFIER) {
        return (struct NamespaceClone){
            .root = clone,
            .last = (struct IdentifierNode**)&clone->rhs,
        };
    } else {
        struct NamespaceClone next = clone_namespace((struct NamespaceAccessNode*)clone->rhs);
        clone->rhs = AS_NODE(next.root);
        return (struct NamespaceClone){
            .root = clone,
            .last = next.last,
        };
    }
}

static void reduce_use_decls(struct UseExprNode *use_decl, struct DeclarationNode **decls)
{
    while (use_decl != null) {
        struct UseExprItem *item = use_decl->items;

        while (item != null) {
            // create namespace access to append
            struct NamespaceAccessNode *root = ALLOC_NODE(struct NamespaceAccessNode);
            *root = (struct NamespaceAccessNode){
                { AST_NAMESPACE_ACCESS, item->ident->node.loc },
                .ident = null,
                .rhs = AS_NODE(item->ident),
            };

            if (use_decl->path->kind == AST_NAMESPACE_ACCESS) {
                struct NamespaceClone namespace = clone_namespace((struct NamespaceAccessNode*)use_decl->path);

                root->ident = *namespace.last;

                // set the previous end to the new access
                *namespace.last = (struct IdentifierNode*)root;
                root = namespace.root;
            } else {
                root->ident = (struct IdentifierNode*)use_decl->path;
            }

            struct DeclarationNode *decl = ALLOC_NODE(struct DeclarationNode);
            *decl = (struct DeclarationNode){
                { AST_DECLARATION, use_decl->node.loc },
                .name = item->ident->src_loc,
                .name_len = item->ident->len,
                .is_global = true,
                .bindings = null,
                .body = AS_NODE(root),
                .next_declaration = *decls,
            };
            *decls = decl;

            item = item->next_item;
        }

        use_decl = use_decl->next_use;
    }
}
static void reduce_use_expr(struct UseExprNode *use_expr)
{
    if (use_expr->expr == null)
        return;

    struct LetExprNode *let_expr = ALLOC_NODE(struct LetExprNode);
    *let_expr = (struct LetExprNode){
        { AST_LET_EXPR, use_expr->node.loc },
        .first_decl = null,
        .body = use_expr->expr,
    };

    reduce_use_decls(use_expr, (struct DeclarationNode**)&let_expr->first_decl);
    use_expr->expr = AS_NODE(let_expr);
}

static void reduce_module(struct ModuleDeclNode *mod)
{
    if (!mod->has_body)
        return;

    reduce_use_decls(mod->use_declarations, &mod->declarations);
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

static void post_reduce_node(struct AstNode *node, void *arg)
{
    if (node == null)
        return;

    if (node->kind == AST_MODULE_DECL) {
        reduce_module((struct ModuleDeclNode*)node);
    } else if (node->kind == AST_USE_EXPR) {
        reduce_use_expr((struct UseExprNode*)node);
    }
}
