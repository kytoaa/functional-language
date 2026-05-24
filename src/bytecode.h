#ifndef func_lang_bytecode_h
#define func_lang_bytecode_h

#include "value.h"
#include "object.h"

enum Bytecode {
    OP_NOOP,
    /// move the value at the top of the stack to the given register
    OP_TRANSFER_STACK_REG,
    /// copy the value at the top of the stack to the given register
    OP_COPY_STACK_REG,
    /// pushes the value in the given register to the stack
    OP_PUSH_REG_STACK,
    /// sets the given register to the value
    /// `op reg u64`
    OP_SET_REG,

    /// swaps the two items at the top of the stack
    OP_SWAP,
    /// creates a copy of the item at the top of the stack
    OP_COPY,

    /// pushes a u64 to the top of the stack
    OP_PUSH_U64,
    /// pops a u64 from the top of the stack
    OP_POP_U64,
    /// adds a value to the u64 at the top of the stack
    /// `op i64`
    OP_U64_ADD,

    /// reads the identifier at an offset down the identifier stack, copying its
    /// value to the top of the stack, 0 being the most recent
    /// `op u16`
    OP_READ_BINDING,
    /// pops the value from the top of the stack, adding it to the identifier stack
    OP_CREATE_BINDING,
    /// pops the item at the top of the ident stack
    OP_REMOVE_BINDING,
    /// pop a number of bindings
    /// `op u8`
    OP_REMOVE_BINDINGS,

    /// unconditional jump, pops the address from the stack
    OP_JUMP,
    /// set the instruction pointer to the value at the given offset down from the stack top
    OP_JUMP_STACK,
    /// jump to the address in the register
    OP_JUMP_REG,

    /// adds or subtracts the given value from the instruction pointer
    /// `op i16`
    OP_JUMP_REL,
    /// adds or subtracts the given value from the instruction pointer if stack pop is true
    /// `op i16`
    OP_JUMP_REL_CONDITIONAL,

    /// jumps to a global function
    /// `op u8`
    OP_JUMP_GLOBALS,

    /// calls the function at the top of the stack
    OP_CALL,
    /// for handling application and calls with greater arguments than arity
    OP_HANDLE_CONTINUATION,
    /// evaluate the item at the top of the stack by calling its eval function
    OP_EVAL,

    /// reads the nth element in a thunk or closure
    /// `op u8`
    OP_DYN_OBJ_READ,

    /// treats the binding as a payload and copies the capture to the stack
    /// `op u16 u8`
    OP_CAPTURE_READ,

    /// reads the objects argument and pushes it to the stack
    /// `op u8`
    /// stack: `[obj]` -> `[obj, arg]`
    OP_OBJECT_READ,

    /// writes a value into a thunk, leaves the evaluated value on the stack
    /// stack: `[thunk, evaluated]` -> `[evaluated]`
    OP_UPDATE_THUNK,

    /// `op u8` arg count
    /// stack: `[..args, f]`
    OP_PARTIAL_APPLY,
    /// creates a closure object with capacity given by the closure info
    /// `op u16` closure info index
    OP_CREATE_CLOSURE,
    /// writes the payload into the closure
    /// stack: `[..payload, closure]`
    OP_WRITE_CLOSURE,

    /// creates an object
    /// `op u16 u16 u16` type_info, variant, arg_count
    OP_CREATE_OBJECT,

    /// creates a thunk object with capacity given by the closure info
    /// `op u16` closure info index
    OP_CREATE_THUNK,
    /// writes the payload into the thunk
    /// stack: `[..payload, thunk]`
    OP_WRITE_THUNK,

    /// creates a cons at the top of the stack
    /// stack: `[r, l]`
    OP_CREATE_CONS,

    /// `op u32` constant;
    OP_PUSH_CONST,

    /// pushes true to the stack
    OP_TRUE,
    /// pushes false to the stack
    OP_FALSE,

    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NOT,
    /// add the items at the top of the stack, commutative
    OP_ADD,
    /// subtracts the items at the top of the stack
    /// expects `l` at top and `r` below for `l * r`
    OP_SUBTRACT,
    /// multiplies the items at the top of the stack, commutative
    OP_MULTIPLY,
    /// divides the items at the top of the stack
    /// expects `l` at top and `r` below for `l / r`
    OP_DIVIDE,
    /// negates the number at the top of the stack
    OP_NEGATE,

    /// checks the item at the top of the stack, pushes a boxed bool
    /// stack: `[val]` -> `[val, result]`
    OP_IS_CONS,
    OP_IS_OBJ,
    OP_IS_INT,
    OP_IS_BOOL,
    OP_IS_CHAR,
    OP_IS_UNIT,

    /// checks if the item at the top of the stack is a specific variant, pushes a boxed bool
    /// op `u16`
    /// stack: `[obj]` -> `[obj, result]`
    OP_IS_VARIANT,

    /// pushes the head of a cons to the top of the stack
    /// stack: `[cons]` -> `[head]`
    OP_HEAD,
    OP_TAIL,

    /// calls an external c function
    /// op u8
    OP_CALL_EXTERN,

    OP_END,
};

enum Register {
    INSTRUCTION_PTR,
    STACK_PTR,
    BINDING_PTR,
    REG_1,
    REG_COUNT,
};

enum VmExternFunction {
    VM_EXTERN_FUNC_WRITE,
    VM_EXTERN_FUNC_WRITE_C_STRING,
    VM_EXTERN_FUNC_READ_CONTENTS,
    VM_EXTERN_FUNC_READ_LINE,
    VM_EXTERN_FUNC_OPEN_FILE,
    VM_EXTERN_FUNC_STDIN,
    VM_EXTERN_FUNC_STDOUT,
    VM_EXTERN_FUNC_STDERR,

    VM_EXTERN_FUNC_SLICE_EMPTY,
    VM_EXTERN_FUNC_SLICE_LEN,
    VM_EXTERN_FUNC_READ_SLICE_INDEX,
    VM_EXTERN_FUNC_SLICE_DROP,
    VM_EXTERN_FUNC_SLICE_TAKE,
    VM_EXTERN_FUNC_SLICE_JOIN,
    VM_EXTERN_FUNC_SLICE_CONS,
    VM_EXTERN_FUNC_SLICE_PUSH,

    VM_EXTERN_FUNC_COUNT,
};

struct ConstantList {
    u64 *ptr;
    u32 len;
    u32 cap;
};
struct ClosureInfoList {
    struct ClosureInfo *ptr;
    u32 len;
    u32 cap;
};

struct Chunk {
    struct {
        u8 *ptr;
        u32 len;
        u32 cap;
    } bytecode;
    struct ConstantList constants;
    struct ClosureInfoList closures;
};

void init_chunk(struct Chunk *chunk);
void chunk_write_byte(struct Chunk *chunk, u8 byte);
void chunk_add_constant(struct Chunk *chunk, struct Value value);

void free_chunk(struct Chunk *chunk);

const char *bytecode_op_name(enum Bytecode byte);

#endif
