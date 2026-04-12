#include "vm.h"
#include "utils.h"
#include "function_call.h"
#include "../prelude.h"

#define DEBUG_CHECKS

struct VM vm;

// defined in [./eval_val.c]
void eval_val();

static void typecheck(Val val, enum ValueType type, const char *error)
{
    for (;;) {
    switch (val->type) {
        case OBJ_THUNK:{
            val = TO_OBJ(((struct Thunk*)val)->evaluated);
            if (val == null)
                panic("unreachable: unevaluated thunk");
            continue;
        }
        case OBJ_BOX:{
            struct Box *box = (struct Box*)val;
            if (box->val.type != type)
                return runtime_error(error);

            return;
        }
        default:
            return runtime_error(error);
    }
    }
}

static enum InterpretResult run_interpreter()
{
next_instruction:

    switch (read_instruction()) {
        case OP_NOOP:
            break;
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

        case OP_READ_BINDING:{
            u16 offset = read_u16() + 1;
        #ifdef DEBUG_CHECKS
            if (offset > vm.registers[BINDING_PTR])
                panic("reading binding at invalid offset");
        #endif
            push_stack(vm.bindings[vm.registers[BINDING_PTR] - offset]);
            break;
        }
        case OP_CREATE_BINDING:{
            u64 value = pop_stack();
            vm.bindings[vm.registers[BINDING_PTR]++] = value;
            break;
        }
        case OP_REMOVE_BINDING:{
        #ifdef DEBUG_CHECKS
            if (vm.registers[BINDING_PTR] == 0)
                panic("no bindings to pop");
        #endif
            vm.registers[BINDING_PTR] -= 1;
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
        case OP_JUMP_REL_CONDITIONAL:{
            i16 jump = (i16)read_u16();
            Val val = pop_val();
            if (val->type != OBJ_BOX || ((struct Box*)val)->val.type != VALUE_BOOL) {
                runtime_error("not a boolean");
                return INTERPRET_RUNTIME_ERROR;
            }
            struct Box *condition = (struct Box*)val;
            if (condition->val.as.boolean) {
                instruction_ptr += jump;
            }
            break;
        }

        case OP_CALL:{
            function_call();
            break;
        }
        case OP_HANDLE_CONTINUATION:{
            handle_continuation();
            break;
        }
        case OP_EVAL:{
            eval_val();
            break;
        }

        case OP_DYN_OBJ_READ:{
            Val dyn_obj = pop_val();
            u8 index = read_instruction();
            if (dyn_obj->type != OBJ_THUNK && dyn_obj->type != OBJ_CLOSURE) {
                runtime_error("not a thunk or closure");
                return INTERPRET_RUNTIME_ERROR;
            }
            struct Box **arguments = obj_dyn_fields(dyn_obj);
            struct Box *value = arguments[index];

            push_val(value);
            break;
        }

        case OP_CREATE_CLOSURE:{
            u16 closure_index = read_u16();
            struct ClosureInfo *closure_info = &vm.code->functions[closure_index];
            struct Closure *closure = obj_create_closure(closure_info);
            struct Box **captures = obj_dyn_fields(TO_OBJ(closure));

            for (u32 i = 0; i < closure_info->capture_count; i++) {
                // reverse order, first free variable is lowest in stack
                captures[closure_info->capture_count - (i + 1)] = (struct Box*)pop_val();
            }
            push_val(closure);
            break;
        }
        case OP_PARTIAL_APPLY:{
            partial_apply();
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
            push_val(result_box);\
        } while (0)
        #define BOOL_BIN_OP(op, arg_type, arg_type_name) do {\
            Val l = pop_val();\
            typecheck(l, arg_type, "expected a " #arg_type_name " for " #op " l arg");\
            Val r = pop_val();\
            typecheck(r, arg_type, "expected a " #arg_type_name " for " #op " r arg");\
            struct Box *l_box = (struct Box*)l;\
            struct Box *r_box = (struct Box*)r;\
                                               \
            bool result = l_box->val.as.integer op r_box->val.as.integer;\
            struct Box *result_box = obj_create_box();\
            result_box->val = BOOL_VAL(result);\
            push_val(result_box);\
        } while (0)
        #define COMPARISON_BIN_OP(op) BOOL_BIN_OP(op, VALUE_INT, number)

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

        case OP_LESS:{
            COMPARISON_BIN_OP(<);
            break;
        }
        case OP_GREATER:{
            COMPARISON_BIN_OP(>);
            break;
        }

        case OP_NOT:{
            Val val = pop_val();
            typecheck(val, VALUE_BOOL, "expected a boolean for `!` argument");
            struct Box *val_box = (struct Box*)val;
            struct Box *result = obj_create_box();
            result->val = BOOL_VAL(!val_box->val.as.boolean);
            push_val(result);
            break;
        }
        case OP_AND:{
            BOOL_BIN_OP(&&, VALUE_BOOL, boolean);
            break;
        }
        case OP_OR:{
            BOOL_BIN_OP(||, VALUE_BOOL, boolean);
            break;
        }

        case OP_HEAD:{
            Val val = pop_val();
            if (val->type != OBJ_CONS) {
                runtime_error("expected a cons");
                return INTERPRET_RUNTIME_ERROR;
            }
            struct Cons *cons = (struct Cons*)val;
            push_val(cons->l);
            break;
        }
        case OP_TAIL:{
            Val val = pop_val();
            if (val->type != OBJ_CONS) {
                runtime_error("expected a cons");
                return INTERPRET_RUNTIME_ERROR;
            }
            struct Cons *cons = (struct Cons*)val;
            push_val(cons->r);
            break;
        }

        case OP_EQUAL:{
            Val l = pop_val();
            //if (l->type != OBJ_BOX)
            Val r = pop_val();

            if (l->type != OBJ_BOX) {
                runtime_error("left argument of `==` is not an equatable value");
                return INTERPRET_RUNTIME_ERROR;
            }
            if (r->type != OBJ_BOX) {
                runtime_error("right argument of `==` is not an equatable value");
                return INTERPRET_RUNTIME_ERROR;
            }

            struct Box *l_box = (struct Box*)l;
            struct Box *r_box = (struct Box*)r;

            if (l_box->val.type != r_box->val.type) {
                runtime_error("arguments of `==` are different types");
                return INTERPRET_RUNTIME_ERROR;
            }

            bool equal = value_equal(l_box->val, r_box->val);
            struct Box *result = obj_create_box();
            result->val = BOOL_VAL(equal);
            push_val(result);
            break;
        }

        case OP_END:
            return INTERPRET_OK;

        #undef COMPARISON_BIN_OP
        #undef BOOL_BIN_OP
        #undef NUM_BIN_OP
    }

    goto next_instruction;

    return INTERPRET_OK;
}

