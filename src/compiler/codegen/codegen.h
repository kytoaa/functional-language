#ifndef func_lang_compiler_codegen_h
#define func_lang_compiler_codegen_h

#include "../../prelude.h"
#include "../../parsing/ast.h"
#include "../../parsing/nodes.h"
#include "../../parsing/ident_table.h"
#include "../../bytecode.h"
#include "../../object.h"

#define IDENT_STACK_SIZE 128

enum CodegenErrorType {
    CODEGEN_ERR_NON_EXISTENT_IDENT,
    CODEGEN_ERR_REDECLARED_GLOBAL,
    CODEGEN_ERR_MAIN_ARGS,
    CODEGEN_ERR_MULTIPLE_MAIN_DECL,
    CODEGEN_ERR_USED_UNDERSCORE,
};

struct CodegenError {
    enum CodegenErrorType type;
    union {
        struct {
            struct Location loc;
        } non_existent_identifier;
        struct {
            struct Location loc;
            struct Location prev_decl_loc;
        } redeclared_global;
        struct {
            struct Location loc;
        } main_args;
        struct {
            struct Location loc;
        } used_underscore;
    } error;
};

struct Identifier {
    const char *ident;
};
struct Global {
    const char *ident;
    struct Location loc;
    u32 constant_index;
};
struct Capture {
    union {
        u8 value;
        struct {
            u8 parent_index : 7;
            bool is_local : 1;
        };
    };
};

struct GlobalList {
    struct Global *ptr;
    u32 len;
    u32 cap;
};
struct CodegenErrorList {
    struct CodegenError *ptr;
    u32 len;
    u32 cap;
};

struct Context {
    struct Context *parent;
    struct IdentifierTable *identifier_table;
    struct Chunk *compiling_chunk;
    struct Identifier ident_stack[IDENT_STACK_SIZE];
    struct Capture capture_stack[IDENT_STACK_SIZE];
    struct GlobalList *globals;
    struct CodegenErrorList *errors;
    u32 ident_stack_len;
    u32 capture_stack_len;
};

void init_context(struct Context *ctx, struct Context *parent);
void end_context(struct Context *ctx);

void non_existent_ident_err(struct Context *ctx, struct Location loc);
void redeclared_global_err(struct Context *ctx, struct Location loc, struct Location prev_decl_loc);
void main_args_err(struct Context *ctx, struct Location loc);
void multiple_main_decl_err(struct Context *ctx, struct Location loc, struct Location prev_decl_loc);
void used_underscore_err(struct Context *ctx, struct Location loc);

struct IdentSearchResult {
    /// ident offset from the top of the binding stack, 0 being top
    u16 offset;
};

void declare_global(struct Context *ctx, const char *ident, u32 constant_index, struct Location loc);
bool resolve_global(struct Context *ctx, const char *ident, u32 *out);

void declare_ident(struct Context *ctx, const char *ident);
void drop_ident(struct Context *ctx, u32 count);

bool get_ident_info(struct Context *ctx, const char *ident, struct IdentSearchResult *out);

bool resolve_capture(struct Context *ctx, const char *ident, u8 *out);

void emit_byte(struct Context *ctx, u8 byte);
void emit_2_bytes(struct Context *ctx, u8 byte, u8 arg);
void emit_u16(struct Context *ctx, u16 value);
void emit_u32(struct Context *ctx, u32 value);
void emit_u64(struct Context *ctx, u64 value);

u32 get_last_bytecode_index(struct Context *ctx);
u8 *get_bytecode_byte(struct Context *ctx, u32 index);

u32 create_constant(struct Context *ctx, enum ObjType type, u32 size);
u64 *get_constant(struct Context *ctx, u32 index);

u16 create_closure_info(struct Context *ctx, struct ClosureInfo info);

#endif
