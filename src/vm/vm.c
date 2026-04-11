#include "vm.h"
#include "utils.h"
#include "../prelude.h"

#define DEBUG_CHECKS

struct VM vm;

// defined in [./eval_val.c]
void eval_val();

static void typecheck(Val val, enum ValueType type, const char *error)
{
    if (val->type != OBJ_BOX)
        return runtime_error(error);

    struct Box *box = (struct Box*)val;
    if (box->val.type != type)
        return runtime_error(error);
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
        case OP_SWAP:{
        #ifdef DEBUG_CHECKS
            if (stack_ptr < 2)
                panic("swap operation with small stack");
        #endif
            u64 top = pop_stack();
            u64 below = pop_stack();
            push_stack(top);
            push_stack(below);
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
        case OP_JUMP:{
            u64 addr = pop_stack();
            instruction_ptr = addr;
            break;
        }
        case OP_JUMP_STACK:{
            u64 offset = read_u64();
            u64 addr = read_stack(offset);
            instruction_ptr = addr;
            break;
        }
        case OP_JUMP_REG:{
            u8 reg = read_reg();
        #ifdef DEBUG_CHECKS
            if (reg == INSTRUCTION_PTR || reg == STACK_PTR)
                panic("invalid register to read for jump");
        #endif
            u64 addr = vm.registers[reg];
            instruction_ptr = addr;
            break;
        }
        case OP_JUMP_REL:{
            i16 jump = (i16)read_u16();
            instruction_ptr += jump;
            break;
        }
        case OP_EVAL:{
            eval_val();
            break;
        }

        #define NUM_BIN_OP(op) do {\
            Val l = pop_val();\
            typecheck(l, VALUE_INT, "expected a number for " #op " l arg");\
            Val r = pop_val();\
            typecheck(r, VALUE_INT, "expected a number for " #op " r arg");\
            struct Box *l_box = (struct Box*)l;\
            struct Box *r_box = (struct Box*)r;\
                                               \
            i32 result = l_box->val.as.integer op r_box->val.as.integer;\
            struct Box *result_box = obj_create_box();\
            result_box->val = INT_VAL(result);\
            push_stack((u64)result_box);\
        } while (0)

        case OP_ADD:{
            NUM_BIN_OP(+);
            break;
        }
        case OP_SUBTRACT:{
            NUM_BIN_OP(-);
            break;
        }
        case OP_MULTIPLY:{
            NUM_BIN_OP(*);
            break;
        }
        case OP_DIVIDE:{
            NUM_BIN_OP(/);
            break;
        }

        #undef NUM_BIN_OP
    }

    return INTERPRET_OK;
}

