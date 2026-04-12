#include "reduction.h"
#include "../prelude.h"
#include "../parsing/nodes.h"
#include "../parsing/traversal.h"

struct Binding {
    const char *ident;
    struct AstNode *origin;
};

struct BoundNames {
    u32 len;
    u8 depth;
    struct Binding bindings[256];
};
static void declare_binding(struct BoundNames *names, struct Binding binding)
{
    if (names->len == 256)
        panic("too many identifiers");
    names->bindings[names->len++] = binding;
}
static void drop_bindings(struct BoundNames *names, u32 number)
{
    if (number > names->len)
        panic("not enough identifiers");
    names->len -= number;
}
static struct AstNode *find_binding(struct BoundNames *names, const char *ident)
{
    for (u32 i = 0; i < names->len; i++) {
        struct Binding *current = &names->bindings[names->len - (i + 1)];

        if (current->ident == ident)
            return current->origin;
    }
    return null;
}

static void reduce_node(struct AstNode *node, void *arg);
static void enter_resolve_names(struct AstNode *node, void *arg);
static void leave_resolve_names(struct AstNode *node, void *arg);

void reduce_ast(struct AstNode *node)
{
    traverse_node(node, null, reduce_node, null);
    struct BoundNames names = { 0, 0, {} };
    traverse_node(node, &names, enter_resolve_names, leave_resolve_names);
}

/// converts declaration to a lambda
static void reduce_declaration(struct DeclarationNode *decl)
{
    struct LambdaNode *lambda = ALLOC_NODE(struct LambdaNode);
    *lambda = (struct LambdaNode){
        .node = { AST_LAMBDA },
        .bindings = decl->bindings,
        .body = decl->body,
    };
    decl->body = AS_NODE(lambda);
    decl->bindings = null;
}

static void reduce_node(struct AstNode *node, void *arg)
{
    if (node == null)
        return;

    if (node->kind == AST_DECLARATION) {
        reduce_declaration((struct DeclarationNode*)node);
    }
}

static void enter_resolve_names(struct AstNode *node, void *arg)
{
    if (node == null)
        return;

    struct BoundNames *names = arg;

    node->depth = names->depth;

    if (node->kind == AST_DECLARATION) {
        struct DeclarationNode *declaration = (struct DeclarationNode*)node;
        names->depth += 1;
    }
    if (node->kind == AST_LAMBDA) {
        struct LambdaNode *lambda = (struct LambdaNode*)node;
        names->depth += 1;
        struct FunctionBindingNode *binding = lambda->bindings;

        while (binding != null) {
            declare_binding(names, (struct Binding){
                .ident = binding->src_loc,
                .origin = AS_NODE(lambda),
            });
            binding = binding->next_binding;
        }
    }
    if (node->kind == AST_IDENTIFIER) {
        struct IdentifierNode *identifier = (struct IdentifierNode*)node;
        struct AstNode *origin = find_binding(names, identifier->src_loc);
        identifier->declaration = origin;
    }
}
static void leave_resolve_names(struct AstNode *node, void *arg)
{
    if (node == null)
        return;

    struct BoundNames *names = arg;

    if (node->kind == AST_DECLARATION) {
        names->depth -= 1;
    }
    if (node->kind == AST_LAMBDA) {
        struct LambdaNode *lambda = (struct LambdaNode*)node;
        names->depth -= 1;
        struct FunctionBindingNode *binding = lambda->bindings;

        while (binding != null) {
            drop_bindings(names, 1);
            binding = binding->next_binding;
        }
    }
}
