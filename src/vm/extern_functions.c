#include "extern_functions.h"
#include "utils.h"
#include <stdio.h>
#include "vm.h"

#define DEBUG_CHECKS

static void print_val(Val val);
static void print_cons(struct Cons *cons);

static void print_value(struct Value val)
{
    switch (val.type) {
        case VALUE_UNIT:
            fprintf(vm.config.out, "()");
            break;
        case VALUE_INT:
            fprintf(vm.config.out, "%d", val.as.integer);
            break;
        case VALUE_BOOL:
            fprintf(vm.config.out, val.as.boolean ? "true" : "false");
            break;
        case VALUE_CHAR:
            fprintf(vm.config.out, "%c", val.as.character);
            break;
        case VALUE_OBJ:
            print_val(val.as.object);
            break;
    }
}

static void print_cons(struct Cons *cons)
{
    fprintf(vm.config.out, "<%p>(", cons);
    print_val(cons->l);
    fprintf(vm.config.out, " :: ");
    print_val(cons->r);
    fprintf(vm.config.out, ")");
}

static void print_val(Val val)
{
    if (val == null)
        panic("null");

    switch (val->type) {
        case OBJ_BOX:
            print_value(((struct Box*)val)->val);
            break;
        case OBJ_CONS:
            print_cons((struct Cons*)val);
            break;
        case OBJ_APPLICATION:
            fprintf(vm.config.out, "<app: %d>", ((struct Application*)val)->arg_count);
            break;
        case OBJ_THUNK:
            fprintf(vm.config.out, "<thunk %p> -> %p", val, ((struct Thunk*)val)->evaluated);
            break;
        case OBJ_CLOSURE:
            fprintf(vm.config.out, "<closure>");
            break;
        default:
            panic("not printable");
            break;
    }
}

void print_stack_val()
{
#ifdef DEBUG_CHECKS
    fprintf(vm.config.out, "OUTPUT :: ");
#endif
    Val val = pop_val();
    print_val(val);
    fprintf(vm.config.out, "\n");
}

void print_c_string()
{
    fprintf(vm.config.error, "%s", (char*)pop_stack());
}

