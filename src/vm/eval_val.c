#include "utils.h"
#include "function_call.h"
#include <stdio.h>

#define set_whnf(value) do { (value)->flags.is_whnf = true; } while (0)

static void eval_box(struct Box *box);
static void eval_thunk(struct Thunk *thunk);
static void eval_application(struct Application *application);

static void eval_value(Val val)
{
    switch (val->type) {
        case OBJ_BOX:
            return eval_box((struct Box*)val);
        case OBJ_THUNK:
            return eval_thunk((struct Thunk*)val);
        case OBJ_APPLICATION:
            return eval_application((struct Application*)val);
        case OBJ_CONS:
            set_whnf(val);
            push_val(val);
            return;
        case OBJ_CLOSURE:
            set_whnf(val);
            push_val(val);
            return;

        case OBJ_TYPE_COUNT:
            panic("unreachable");
            return;
    }
}

void eval_val()
{
    Val val = pop_val();

    if (IS_WHNF(val)) {
        push_val(val);
        return;
    }
    eval_value(val);
}

static void eval_box(struct Box *box)
{
    switch (box->val.type) {
        case VALUE_OBJ:
            return eval_value(box->val.as.object);
        default:
            set_whnf(&box->obj);
            push_val(box);
            return;
    }
}

static void eval_thunk(struct Thunk *thunk)
{
    if (thunk->evaluated != null) {
        return eval_value(thunk->evaluated);
    }
    struct Box **payload = obj_dyn_fields(TO_OBJ(thunk));
    u64 addr = thunk->info->address;
    vm.registers[REG_1] = (u64)payload;

    // push continuation, pushes eval op to loop until whnf
    push_stack(instruction_ptr - 1);
    push_val(thunk);
    // continue to update thunk
    push_stack(address_of_global(GLOBAL_FUNC_UPDATE_THUNK));
    jump(addr);
    // resume execution
}

static void eval_application(struct Application *application)
{
    struct Box **payload = obj_dyn_fields(TO_OBJ(application));

    Val closure = application->closure;
    if (closure->type != OBJ_CLOSURE) {
        printf("\n%d\n", closure->type);
        panic("not a closure");
    }

    if (application->arg_count >= ((struct Closure*)application->closure)->info->arity) {
        push_stack(instruction_ptr - 1);
        push_val(application);
        function_call();
    } else {
        set_whnf(&application->obj);
        push_val(application);
    }
}

#undef set_whnf
