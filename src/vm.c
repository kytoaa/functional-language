#include "vm.h"
#include "bytecode.h"
#include "object.h"
#include "prelude.h"

#define DEBUG_CHECKS

struct VM vm;

#define instruction_ptr() vm.registers[INSTRUCTION_PTR]
#define stack_ptr() vm.registers[STACK_PTR]

static inline u8 read_instruction()
{
    return vm.code->instructions[vm.registers[INSTRUCTION_PTR]++];
}
static inline u8 read_reg()
{
    u8 reg = read_instruction();
#ifdef DEBUG_CHECKS
    if (reg >= REG_COUNT)
        panic("invalid register\n");
#endif
    return reg;
}
static inline u64 read_u64()
{
    // ip may not be aligned so using union to deal with unaligned read
    union {
        u8 bytes[sizeof(u64)];
        u64 as_u64;
    } values;

    for (u8 i = 0; i < sizeof(u64); i++) {
        values.bytes[i] = read_instruction();
    }

    return values.as_u64;
}
static inline u16 read_u16()
{
    // ip may not be aligned so using union to deal with unaligned read
    union {
        u8 bytes[sizeof(u16)];
        u16 as_u16;
    } values;

    for (u8 i = 0; i < sizeof(u16); i++) {
        values.bytes[i] = read_instruction();
    }

    return values.as_u16;
}

static inline u64 read_stack(u64 offset)
{
#ifdef DEBUG_CHECKS
    if (offset >= stack_ptr())
        panic("offset larger than stack");
#endif
    // stack ptr points to next free spot, offset 0 should point
    // to value at top so have to increase offset by 1
    return vm.stack[stack_ptr() - (offset + 1)];
}

static inline u64 pop_stack()
{
    return vm.stack[--stack_ptr()];
}
static inline void push_stack(u64 val)
{
    vm.stack[instruction_ptr()++] = val;
}
static inline struct Value pop_value()
{
    // round up so read aligned to u64
    const u32 offset = ((sizeof(struct Value) - 1) / sizeof(u64)) + 1;
#ifdef DEBUG_CHECKS
    if (stack_ptr() < offset)
        panic("stack too small to hold `struct Value`");
#endif
    struct Value *val = (struct Value*)&vm.stack[stack_ptr() - offset];
    stack_ptr() -= offset;
    return *val;
}
static inline void push_value(struct Value value)
{
    // has to round up to ensure enough room
    const u32 size = ((sizeof(struct Value) - 1) / sizeof(u64)) + 1;
    struct Value *val = (struct Value*)&vm.stack[stack_ptr()];
    *val = value;
    stack_ptr() += size;
}

static void runtime_error(const char *msg)
{
    fprintf(vm.config.error, "error: %s\n", msg);
#ifdef DEBUG_CHECKS
    fprintf(vm.config.error, "\tip: [%lu], sp: [%lu], r1: [%lu]\n", instruction_ptr(), stack_ptr(), vm.registers[REG_1]);
#endif
}

static enum InterpretResult run_instruction()
{
    switch (read_instruction()) {
        case OP_TRANSFER_STACK_REG:{
            u8 reg = read_reg();
            u64 val = pop_stack();
            vm.registers[reg] = val;
            break;
        }
        case OP_PUSH_REG_STACK:{
            u8 reg = read_reg();
            u64 val = vm.registers[reg];
            push_stack(val);
            break;
        }
        case OP_SET_REG:{
            u8 reg = read_reg();
            u64 val = read_u64();
            vm.registers[reg] = val;
            break;
        }
        case OP_PUSH_U64:{
            u64 val = read_u64();
            push_stack(val);
            break;
        }
        case OP_POP_U64:{
            pop_stack();
            break;
        }
        case OP_JUMP_STACK:{
            u64 offset = read_u64();
            u64 addr = read_stack(offset);
            instruction_ptr() = addr;
        }
        case OP_JUMP_REG:{
            u8 reg = read_reg();
        #ifdef DEBUG_CHECKS
            if (reg == INSTRUCTION_PTR || reg == STACK_PTR)
                panic("invalid register to read for jump");
        #endif
            u64 addr = vm.registers[reg];
            instruction_ptr() = addr;
            break;
        }
        case OP_JUMP_REL:{
            i16 jump = (i16)read_u16();
            instruction_ptr() += jump;
            break;
        }
        case OP_PARTIAL_APPLY:{
            struct Value function = pop_value();
            u8 arg_count = pop_stack();
            u8 extra_args = 0;
            u8 arity = 0;

            if (!IS_OBJ(function)) {
                runtime_error("expected a function");
                return INTERPRET_RUNTIME_ERROR;
            }
            switch (OBJ_TYPE(function)) {
                case OBJ_CLOSURE:{
                    struct Closure *closure = (struct Closure*)AS_OBJ(function);
                    arity = closure->info->arity;
                    break;
                }
                case OBJ_APPLICATION:{
                    struct Application *applying_to = (struct Application*)AS_OBJ(function);
                    extra_args = applying_to->arg_count;
                    arity = applying_to->arity;
                    break;
                }
                default:{
                    runtime_error("not a function");
                    return INTERPRET_RUNTIME_ERROR;
                }
            }

            if (arity < arg_count) {
                runtime_error("too many arguments applied");
                return INTERPRET_RUNTIME_ERROR;
            }

            struct Application *application = obj_create_application(arg_count);
            application->arg_count = arg_count + extra_args;
            application->closure = AS_OBJ(function);
            application->arity = arity - arg_count;

            struct Box **dyn_fields = obj_dyn_fields(TO_OBJ(application));

            for (u32 i = 0; i < arg_count; i++) {
                dyn_fields[i + extra_args] = (struct Box*)pop_stack();
            }
            if (extra_args > 0) {
                struct Application *applying_to = (struct Application*)AS_OBJ(function);
                struct Box **prev_dyn_fields = obj_dyn_fields(TO_OBJ(applying_to));
                for (u32 i = 0; i < extra_args; i++) {
                    dyn_fields[i] = prev_dyn_fields[i];
                }
                application->closure = applying_to->closure;
            }
            push_value(OBJ_VAL(TO_OBJ(application)));
            break;
        }

        case OP_CALL:{
            struct Value function = pop_value();
            if (!IS_OBJ(function)) {
                runtime_error("expected a function");
                return INTERPRET_RUNTIME_ERROR;
            }
            push_stack(instruction_ptr() + 1);
            struct Closure *closure;

            switch (OBJ_TYPE(function)) {
                case OBJ_CLOSURE:{
                    closure = (struct Closure*)AS_OBJ(function);
                    break;
                }
                case OBJ_APPLICATION:{
                    struct Application *application = (struct Application*)AS_OBJ(function);
                    struct Box **arguments = obj_dyn_fields(TO_OBJ(application));
                    for (u32 i = 0; i < application->arg_count; i++) {
                        push_stack((u64)arguments[i]);
                    }
                    closure = (struct Closure*)application->closure;
                    break;
                }
                default:{
                    runtime_error("not a function");
                    return INTERPRET_RUNTIME_ERROR;
                }
            }

            vm.registers[REG_1] = (u64)closure;
            instruction_ptr() = closure->info->address;
            break;
        }
    }

    return INTERPRET_OK;
}

