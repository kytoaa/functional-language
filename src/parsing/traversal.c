#include "traversal.h"
#include "ast.h"
#include "nodes.h"

void traverse_node(
    struct AstNode *n,
    void *arg,
    void (*pre_callback)(struct AstNode*, void*),
    void (*post_callback)(struct AstNode*, void*)
) {
    if (n == null)
        return;

    if (pre_callback != null)
        pre_callback(n, arg);

    switch (n->kind) {
        case AST_MODULE_DECL:{
            struct ModuleDeclNode *node = (struct ModuleDeclNode*)n;
            traverse_node(AS_NODE(node->name), arg, pre_callback, post_callback);

            if (!node->has_body && node->declarations != null) {
                traverse_node(AS_NODE(node->declarations), arg, pre_callback, post_callback);
            } else {
                struct DeclarationNode *decl = node->declarations;
                while (decl != null) {
                    traverse_node(AS_NODE(decl), arg, pre_callback, post_callback);
                    decl = decl->next_declaration;
                }
            }
            struct UseDeclNode *use_decl = node->use_declarations;
            while (use_decl != null) {
                traverse_node(AS_NODE(use_decl), arg, pre_callback, post_callback);
                use_decl = use_decl->next_use;
            }

            struct ModuleDeclNode *submodule = node->submodules;
            while (submodule != null) {
                traverse_node(AS_NODE(submodule), arg, pre_callback, post_callback);
                submodule = submodule->next_mod;
            }
            break;
        }
        case AST_USE_DECL:{
            struct UseDeclNode *node = (struct UseDeclNode*)n;
            traverse_node(AS_NODE(node->use_expr), arg, pre_callback, post_callback);
            break;
        }
        case AST_NAMESPACE_ACCESS:{
            struct NamespaceAccessNode *node = (struct NamespaceAccessNode*)n;
            traverse_node(AS_NODE(node->ident), arg, pre_callback, post_callback);
            traverse_node(node->rhs, arg, pre_callback, post_callback);
            break;
        }
        case AST_APPLICATION:{
            struct ApplicationNode *node = (struct ApplicationNode*)n;
            traverse_node(node->function, arg, pre_callback, post_callback);
            traverse_node(node->argument, arg, pre_callback, post_callback);
            break;
        }
        case AST_BIN_OP:{
            struct BinOpNode *node = (struct BinOpNode*)n;
            traverse_node(node->l, arg, pre_callback, post_callback);
            traverse_node(node->r, arg, pre_callback, post_callback);
            break;
        }
        case AST_UNARY_OP:{
            struct UnaryOpNode *node = (struct UnaryOpNode*)n;
            traverse_node(node->val, arg, pre_callback, post_callback);
            break;
        }
        case AST_DECLARATION:{
            struct DeclarationNode *node = (struct DeclarationNode*)n;
            struct FunctionBindingNode *binding = node->bindings;
            while (binding != null) {
                traverse_node(AS_NODE(binding), arg, pre_callback, post_callback);
                binding = binding->next_binding;
            }
            traverse_node(node->body, arg, pre_callback, post_callback);
            break;
        }
        case AST_IF_EXPR:{
            struct IfExprNode *node = (struct IfExprNode*)n;
            traverse_node(node->condition, arg, pre_callback, post_callback);
            traverse_node(node->then_expr, arg, pre_callback, post_callback);
            traverse_node(node->else_expr, arg, pre_callback, post_callback);
            break;
        }
        case AST_LET_EXPR:{
            struct LetExprNode *node = (struct LetExprNode*)n;
            struct DeclarationNode *decl = (struct DeclarationNode*)node->first_decl;
            while (decl != null) {
                traverse_node(AS_NODE(decl), arg, pre_callback, post_callback);
                decl = decl->next_declaration;
            }
            traverse_node(node->body, arg, pre_callback, post_callback);
            break;
        }
        case AST_LAMBDA:{
            struct LambdaNode *node = (struct LambdaNode*)n;
            struct FunctionBindingNode *binding = node->bindings;
            while (binding != null) {
                traverse_node(AS_NODE(binding), arg, pre_callback, post_callback);
                binding = binding->next_binding;
            }
            traverse_node(node->body, arg, pre_callback, post_callback);
            break;
        }
        case AST_CASE_EXPR:{
            struct CaseExprNode *node = (struct CaseExprNode*)n;
            traverse_node(node->value, arg, pre_callback, post_callback);

            struct CasePatternNode *pattern = (struct CasePatternNode*)node->first_pattern;
            while (pattern != null) {
                traverse_node(AS_NODE(pattern), arg, pre_callback, post_callback);
                pattern = pattern->next_pattern;
            }
            break;
        }
        case AST_CASE_PATTERN:{
            struct CasePatternNode *pattern = (struct CasePatternNode*)n;
            traverse_node(pattern->pattern, arg, pre_callback, post_callback);
            traverse_node(pattern->condition, arg, pre_callback, post_callback);
            traverse_node(pattern->body, arg, pre_callback, post_callback);
            break;
        }
        case AST_CONSTRUCTOR:{
            struct ConstructorNode *node = (struct ConstructorNode*)n;
            traverse_node(AS_NODE(node->body), arg, pre_callback, post_callback);
            break;
        }
        case AST_ATTR:{
            struct AttributeNode *node = (struct AttributeNode*)n;
            traverse_node(AS_NODE(node->ident), arg, pre_callback, post_callback);
            traverse_node(AS_NODE(node->body), arg, pre_callback, post_callback);
            break;
        }
        default:
            break;
    }

    if (post_callback != null)
        post_callback(n, arg);
}
