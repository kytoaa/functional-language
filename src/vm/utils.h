#ifndef func_lang_vm_utils_h
#define func_lang_vm_utils_h

#include "../prelude.h"
#include "vm.h"

#define instruction_ptr vm.registers[INSTRUCTION_PTR]
#define stack_ptr vm.registers[STACK_PTR]

void runtime_error(const char *msg);
void print_stack(FILE *out);

u8 read_instruction();
u8 read_reg();
u64 read_u64();
u16 read_u16();
u64 read_stack(u64 offset);

u64 pop_stack();
void push_stack(u64 val);

#define pop_val() (Val)(pop_stack())
#define push_val(val) do { push_stack((u64)(val)); } while (0)
#define jump(addr) do { printf("jumping to %llu\n", addr); instruction_ptr = addr; } while (0)

#endif
