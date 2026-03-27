#include "ast.h"

static struct AstAllocator allocator = { .top = allocator.mem + AST_ALLOC_SIZE };
static struct AstAllocator *current_allocator = &allocator;

static void init_ast_allocator(struct AstAllocator *allocator)
{
    allocator->top = allocator->mem + AST_ALLOC_SIZE;
    allocator->next = null;
}
static void new_ast_allocator()
{
    struct AstAllocator *new_allocator = alloc_mem(sizeof(struct AstAllocator));
    init_ast_allocator(new_allocator);
    current_allocator->next = new_allocator;
    current_allocator = new_allocator;
}
static void free_ast_allocators()
{
    struct AstAllocator *current = allocator.next;
    struct AstAllocator *next;

    while (current != null) {
        next = current->next;
        free_mem(current);
        current = next;
    }
}
struct AstNode *alloc_ast_node(usize size)
{
    u8 *new_ptr = current_allocator->top - size;
    if (new_ptr < current_allocator->mem) {
        new_ast_allocator();
        new_ptr = current_allocator->top - size;
    }
    current_allocator->top = new_ptr;

    return (struct AstNode*)new_ptr;
}

