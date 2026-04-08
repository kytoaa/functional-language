#include "dead_code.h"
#include "../../parsing/traversal.h"

struct Item {
    const char *ident;
    struct AstNode *decl;
    u32 uses;
};

struct EnvTracker {
    struct Item *items;
    u32 len;
    u32 cap;
};

static void add_decl_to_env(struct EnvTracker *env, const char *ident, struct AstNode *decl)
{
    if (env->len == env->cap) {
        u32 new_cap = (env->cap == 0) ? 4 : env->cap * 2;
        struct Item *new_ptr = realloc_mem(env->items, new_cap * sizeof(struct Item));
        env->items = new_ptr;
        env->cap = new_cap;
    }
    env->items[env->len] = (struct Item){
        .ident = ident,
        .decl = decl,
        .uses = 0,
    };
    env->len += 1;
}
static void use_ident(struct EnvTracker *env, const char *ident)
{
    for (u32 i = 0; i < env->len; i++) {
        struct Item *item = &env->items[env->len - (i + 1)];
        if (item->ident == ident) {
            item->uses += 1;
            return;
        }
    }
}

static void search_function(struct DeclarationNode *function);
static void enter_expr(struct AstNode *expr, void *environment);
static void leave_expr(struct AstNode *expr, void *environment);

void remove_dead_code(struct DeclarationNode *root)
{
}

static void search_function(struct DeclarationNode *function)
{
    struct EnvTracker env = {};
    struct FunctionBindingNode *binding = function->bindings;
    while (binding != null) {
        add_decl_to_env(&env, binding->src_loc, AS_NODE(binding));
        binding = binding->next_binding;
    }
    traverse_node(function->body, &env, enter_expr, leave_expr);
}

static void enter_expr(struct AstNode *expr, void *environment)
{
    struct EnvTracker *env = environment;
}

static void leave_expr(struct AstNode *expr, void *environment)
{
    struct EnvTracker *env = environment;
}

