#include "codegen.h"

struct FreeVariableList {
    const char **ptr;
    u32 len;
    u32 cap;
};

static void add_free_variable(struct FreeVariableList *fv_list, const char *fv)
{
    if (fv_list->len == fv_list->cap) {
        u32 new_cap = (fv_list->cap == 0) ? 2 : fv_list->cap * 2;
        const char **new_ptr = realloc_mem(fv_list->ptr, new_cap * sizeof(const char*));
        fv_list->ptr = new_ptr;
        fv_list->cap = new_cap;
    }
    fv_list->ptr[fv_list->len++] = fv;
}

void get_free_variables(struct LambdaNode *lambda)
{
}
