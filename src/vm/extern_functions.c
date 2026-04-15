#include "extern_functions.h"
#include "utils.h"
#include <stdio.h>

#define DEBUG_CHECKS

static void print_val(Val val);

static void print_value(struct Value val)
{
    switch (val.type) {
        case VALUE_UNIT:
            printf("()");
            break;
        case VALUE_INT:
            printf("%d", val.as.integer);
            break;
        case VALUE_BOOL:
            printf(val.as.boolean ? "true" : "false");
            break;
        case VALUE_CHAR:
            printf("%c", val.as.character);
            break;
        case VALUE_OBJ:
            print_val(val.as.object);
            break;
    }
}

static void print_val(Val val)
{
    if (val == null)
        panic("null");

    switch (val->type) {
        case OBJ_BOX:
            print_value(((struct Box*)val)->val);
            break;
        default:
            printf("%d\n", val->type);
            panic("not printable");
            break;
    }
}

void print_stack_val()
{
#ifdef DEBUG_CHECKS
    printf("OUTPUT :: ");
#endif
    Val val = pop_val();
    print_val(val);
    printf("\n");
}
