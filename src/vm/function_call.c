#include "utils.h"
#include "../compiler/builtins.h"
#include <stdio.h>

static void jump_to_closure(struct Closure *closure)
{
    vm.registers[REG_1] = (u64)closure;
    instruction_ptr = closure->info->address;
}

static void exact_arg_call(struct Application *application, struct Closure *closure)
{
    struct Box **payload = obj_dyn_fields(TO_OBJ(application));

    for (u32 i = 0; i < application->arg_count; i++) {
        push_stack((u64)payload[application->arg_count - (i + 1)]);
    }

    jump_to_closure(closure);
}

static void application_call(Val application_val)
{
    struct Application *application = (struct Application*)application_val;

    if (application->closure->type != OBJ_CLOSURE)
        return runtime_error("not a valid application");

    struct Closure *closure = (struct Closure*)application->closure;

    if (application->arg_count < closure->info->arity) {
        push_stack((u64)application);
        return;
    } else if (application->arg_count == closure->info->arity) {
        exact_arg_call(application, closure);
    } else {
        // `application->arg_count > closure->info->arity`
        struct Box **payload = obj_dyn_fields(TO_OBJ(application));

        for (u32 i = 0; i < application->arg_count; i++) {
            if (application->arg_count - i == closure->info->arity) {
                // push remaining args to the stack
                push_stack(i);
                push_stack(address_of_global(GLOBAL_FUNC_APPL_CONT));
            }
            push_stack((u64)payload[application->arg_count - (i + 1)]);
        }

        jump_to_closure(closure);
    }
}

void function_call()
{
    Val function_val = pop_val();

    switch (function_val->type) {
        case OBJ_APPLICATION:
            return application_call(function_val);
        case OBJ_THUNK:
            return;
        default:
            printf("\n%d\n", function_val->type);
            return runtime_error("expected a function");
    }

}

void handle_continuation()
{
    // stack: `[..cont args f]`

    Val evaluated_function = pop_val();
    u64 remaining_args = pop_stack();

    struct Closure *closure = null;

    switch (evaluated_function->type) {
        case OBJ_APPLICATION:{
            struct Application *application = (struct Application*)evaluated_function;
            struct Box **payload = obj_dyn_fields(TO_OBJ(application));

            for (u32 i = 0; i < application->arg_count; i++) {
                push_stack((u64)payload[application->arg_count - (i + 1)]);
            }
            remaining_args += application->arg_count;
            closure = (struct Closure*)application->closure;

            break;
        }
        case OBJ_CLOSURE:{
            closure = (struct Closure*)evaluated_function;
            break;
        }
        default:
            printf("\n%d\n", evaluated_function->type);
            return runtime_error("expected a function");
    }

    if (remaining_args < closure->info->arity) {
        printf("\nargs < arity :: %llu < %u\n", remaining_args, closure->info->arity);
        struct Application *constructed_appl = obj_create_application(remaining_args);
        constructed_appl->closure = TO_OBJ(closure);
        struct Box **new_payload = obj_dyn_fields(TO_OBJ(constructed_appl));

        for (u32 i = 0; i < remaining_args; i++) {
            new_payload[i] = (struct Box*)pop_val();
        }
    } else if (remaining_args == closure->info->arity) {
        printf("\nargs == arity :: %llu == %u\n", remaining_args, closure->info->arity);
        // args are on stack in correct order
        jump_to_closure(closure);
    } else {
        printf("\nargs > arity :: %llu > %u\n", remaining_args, closure->info->arity);
        // `remaining_args > closure->info->arity`
        u32 extra_args = remaining_args - closure->info->arity;

        // make space on the stack
        for (u32 i = 1; i <= closure->info->arity; i++) {
            vm.stack[stack_ptr - i + 2] = vm.stack[stack_ptr - i];
        }

        // push the next continuation
        vm.stack[stack_ptr - closure->info->arity] = extra_args;
        vm.stack[stack_ptr - closure->info->arity + 1] = address_of_global(GLOBAL_FUNC_APPL_CONT);

        // move back to top
        stack_ptr += 2;

        jump_to_closure(closure);
    }
}

void partial_apply()
{
    u8 arg_count = read_instruction();
    u8 extra_args = 0;
    
    Val function = pop_val();
    struct Closure *closure = null;
    switch (function->type) {
        case OBJ_APPLICATION:{
            struct Application *application = (struct Application*)function;
            extra_args = application->arg_count;
            closure = (struct Closure*)application->closure;
            break;
        }
        case OBJ_CLOSURE:
            closure = (struct Closure*)function;
            break;
        default:
            printf("\n%d\n", function->type);
            return runtime_error("expected a function");
    }

    struct Application *application = obj_create_application(arg_count + extra_args);
    struct Box **payload = obj_dyn_fields(TO_OBJ(application));

    if (extra_args > 0) {
        struct Box **prev_args = obj_dyn_fields(function);

        for (u32 i = 0; i < extra_args; i++) {
            payload[i] = prev_args[i];
        }
    }
    for (u32 i = 0; i < arg_count; i++) {
        payload[i + extra_args] = (struct Box*)pop_val();
    }

    application->closure = TO_OBJ(closure);
    application->arg_count = arg_count + extra_args;

    push_val(application);
}
