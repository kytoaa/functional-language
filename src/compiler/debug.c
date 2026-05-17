#include "debug.h"
#include <stdio.h>

static void print_register(FILE *out, u8 reg)
{
    switch (reg) {
        case INSTRUCTION_PTR:
            fprintf(out, " ip");
            break;
        case STACK_PTR:
            fprintf(out, " sp");
            break;
        case BINDING_PTR:
            fprintf(out, " bp");
            break;
        case REG_1:
            fprintf(out, " r1");
            break;
        default:
            return panic("not a register");
    }
}

static u64 read_u64(u8 *bytes)
{
    return (union { u64 u64; u8 bytes[8]; }){
        .bytes = {
            bytes[0],
            bytes[1],
            bytes[2],
            bytes[3],
            bytes[4],
            bytes[5],
            bytes[6],
            bytes[7],
        },
    }.u64;
}
static u16 read_u16(u8 *bytes)
{
    return (union { u16 u16; u8 bytes[2]; }){
        .bytes = {
            bytes[0],
            bytes[1],
        },
    }.u16;
}
static u32 read_u32(u8 *bytes)
{
    return (union { u32 u32; u8 bytes[4]; }){
        .bytes = {
            bytes[0],
            bytes[1],
            bytes[2],
            bytes[3],
        },
    }.u32;
}

static const char *extern_function_name(enum VmExternFunction func)
{
    switch (func) {
        case VM_EXTERN_FUNC_WRITE:
            return "WRITE";
        case VM_EXTERN_FUNC_WRITE_C_STRING:
            return "WRITE_C_STRING";
        case VM_EXTERN_FUNC_READ_CONTENTS:
            return "READ";
        case VM_EXTERN_FUNC_READ_LINE:
            return "READ_LINE";
        case VM_EXTERN_FUNC_OPEN_FILE:
            return "OPEN_FILE";
        case VM_EXTERN_FUNC_STDIN:
            return "STDIN";
        case VM_EXTERN_FUNC_STDOUT:
            return "STDOUT";
        case VM_EXTERN_FUNC_STDERR:
            return "STDERR";

        case VM_EXTERN_FUNC_SLICE_EMPTY:
            return "SLICE_EMPTY";
        case VM_EXTERN_FUNC_SLICE_LEN:
            return "SLICE_LEN";
        case VM_EXTERN_FUNC_READ_SLICE_INDEX:
            return "READ_SLICE_INDEX";
        case VM_EXTERN_FUNC_SLICE_DROP:
            return "SLICE_DROP";
        case VM_EXTERN_FUNC_SLICE_TAKE:
            return "SLICE_TAKE";
        case VM_EXTERN_FUNC_SLICE_JOIN:
            return "SLICE_JOIN";
        case VM_EXTERN_FUNC_SLICE_CONS:
            return "SLICE_CONS";
        case VM_EXTERN_FUNC_SLICE_PUSH:
            return "SLICE_PUSH";

        case VM_EXTERN_FUNC_COUNT:
            return "COUNT";

        default:
            return "unknown extern function";
    }
}

u32 print_instruction(FILE *out, u8 *bytes)
{
    const char *op_name = bytecode_op_name(bytes[0]);
    if (op_name == null) {
        printf("\n%d\n", bytes[0]);
        panic("invalid instruction");
    }
    fprintf(out, "%s", op_name);

    u32 consumed = 1;

    switch (bytes[0]) {
        case OP_TRANSFER_STACK_REG:
        case OP_COPY_STACK_REG:
        case OP_PUSH_REG_STACK:
        case OP_JUMP_REG:
            print_register(out, bytes[1]);
            consumed++;
            break;
        case OP_SET_REG:
            print_register(out, bytes[1]);
            consumed += 1;
        case OP_PUSH_U64:{
            u64 value = read_u64(&bytes[consumed]);
            consumed += 8;
            fprintf(out, " %llu", value);
            break;
        }

        case OP_READ_BINDING:
        case OP_CREATE_CLOSURE:
        case OP_CREATE_THUNK:{
            u16 u16 = read_u16(&bytes[1]);
            consumed += 2;
            fprintf(out, " %d", u16);
            break;
        }
        case OP_REMOVE_BINDINGS:{
            u8 u8 = bytes[1];
            consumed += 1;
            fprintf(out, " %d", u8);
            break;
        }

        case OP_JUMP_REL:
        case OP_JUMP_REL_CONDITIONAL:{
            i16 jump = (i16)read_u16(&bytes[1]);
            consumed += 2;
            fprintf(out, " %d", jump);
            break;
        }
        case OP_JUMP_GLOBALS:
        case OP_DYN_OBJ_READ:
        case OP_PARTIAL_APPLY:
            fprintf(out, " %d", bytes[1]);
            consumed += 1;
            break;
        case OP_CAPTURE_READ:{
            u16 u16 = read_u16(&bytes[1]);
            fprintf(out, " %d %d", u16, bytes[3]);
            consumed += 3;
            break;
        }
        case OP_CALL_EXTERN:{
            enum VmExternFunction function = bytes[1];
            consumed += 1;
            fprintf(out, " %s", extern_function_name(function));
            break;
        }
        case OP_U64_ADD:{
            i64 val = (i64)read_u64(&bytes[1]);
            consumed += 8;
            fprintf(out, " %lld", val);
            break;
        }
        case OP_PUSH_CONST:{
            u32 val = read_u32(&bytes[1]);
            consumed += 4;
            fprintf(out, " %u", val);
            break;
        }
    }

    fprintf(out, "\n");
    return consumed;
}

void print_instructions(FILE *output, const struct Chunk *chunk)
{
    u32 i = 0;
    while (i < chunk->bytecode.len) {
        fprintf(output, "%-2d | ", i);
        i += print_instruction(output, &chunk->bytecode.ptr[i]);
    }
}
