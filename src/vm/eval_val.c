#include "utils.h"

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
            return push_stack((u64)val);
        case OBJ_CLOSURE:
            set_whnf(val);
            return push_stack((u64)val);

        case OBJ_TYPE_COUNT:
            panic("unreachable");
            return;
    }
}

void eval_val()
{
    Val val = pop_val();

    if (IS_WHNF(val)) {
        push_stack((u64)val);
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
            return push_stack((u64)box);
    }
}

static void eval_thunk(struct Thunk *thunk)
{
    if (thunk->evaluated != null) {
        return eval_box(thunk->evaluated);
    }
    struct Box **payload = obj_dyn_fields(TO_OBJ(thunk));
    u64 addr = (u64)payload[0];
    vm.registers[REG_1] = (u64)&payload[1];
    // push continuation, pushes eval op to loop until whnf
    push_stack(instruction_ptr - 1);
    jump(addr);
    // resume execution
}

static void eval_application(struct Application *application)
{
    struct Box **payload = obj_dyn_fields(TO_OBJ(application));

    for (u32 i = 0; i < application->arg_count; i++) {
        // push in reverse order so most recently applied arg is bottom
        // ensures correct behaviour with nested applications
        push_stack((u64)payload[application->arg_count - (i + 1)]);
    }

    // closure is evaluated and arguments are on stack in expected order
    eval_value(application->closure);
}

#undef set_whnf
