#include "utils.h"

#include <stdio.h>

#define DEBUG_CHECKS

void print_stack(FILE *out)
{
    fprintf(out, "%llu, %llu [ ", instruction_ptr, vm.registers[REG_1]);
    if (stack_ptr > 0) {
        for (u32 i = 0; i < stack_ptr - 1; i++) {
            fprintf(out, "%llu, ", vm.stack[i]);
        }
        fprintf(out, "%llu ]\n", vm.stack[stack_ptr - 1]);
    } else {
        fprintf(out, "]\n");
    }
}

void runtime_error(const char *msg)
{
    fprintf(vm.config.error, "error: %s\n", msg);
#ifdef DEBUG_CHECKS
    fprintf(vm.config.error, "\tip: [%llu], sp: [%llu], r1: [%llu]\n", instruction_ptr, stack_ptr, vm.registers[REG_1]);
    print_stack(vm.config.error);
#endif
}

u8 read_instruction()
{
    return vm.code.instructions[vm.registers[INSTRUCTION_PTR]++];
}
u8 read_reg()
{
    u8 reg = read_instruction();
#ifdef DEBUG_CHECKS
    if (reg >= REG_COUNT)
        panic("invalid register\n");
#endif
    return reg;
}
u64 read_u64()
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
u16 read_u16()
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

u64 read_stack(u64 offset)
{
#ifdef DEBUG_CHECKS
    if (offset >= stack_ptr)
        panic("offset larger than stack");
#endif
    // stack ptr points to next free spot, offset 0 should point
    // to value at top so have to increase offset by 1
    return vm.stack[stack_ptr - (offset + 1)];
}

u64 pop_stack()
{
    return vm.stack[--stack_ptr];
}
void push_stack(u64 val)
{
    vm.stack[stack_ptr++] = val;
    if (val == 2) {
        runtime_error("pushed 2");
    }
}

u64 address_of_global(enum GlobalFunction global)
{
    return vm.code.global_function_start + global_function_offset(global);
}
