#ifndef func_lang_compiler_codegen_h
#define func_lang_compiler_codegen_h

#include "../../prelude.h"
#include "../../parsing/ast.h"
#include "../../parsing/nodes.h"
#include "../../parsing/ident_table.h"
#include "../../bytecode.h"
#include "../../object.h"

#define IDENT_STACK_SIZE 256

struct Identifier {
    const char *ident;
    u32 function;
    u32 depth;
};

struct Context {
    struct IdentifierTable *identifier_table;
    struct Chunk *compiling_chunk;
    struct Identifier ident_stack[IDENT_STACK_SIZE];
    u32 ident_stack_len;
    u32 current_depth;
};

struct IdentSearchResult {
    u16 offset;
    u16 function;
};

void declare_ident(struct Context *ctx, const char *ident);

/// returns the ident's offset from the top of the stack
struct IdentSearchResult get_ident_offset(struct Context *ctx, const char *ident);

void emit_byte(struct Context *ctx, u8 byte);
void emit_2_bytes(struct Context *ctx, u8 byte, u8 arg);
void emit_u16(struct Context *ctx, u16 value);
void emit_u32(struct Context *ctx, u32 value);
u32 get_last_bytecode_index(struct Context *ctx);
u8 *get_bytecode_byte(struct Context *ctx, u32 index);

u32 create_constant(struct Context *ctx, enum ObjType type, u32 size);
u64 *get_constant(struct Context *ctx, u32 index);

u16 create_closure_info(struct Context *ctx, struct ClosureInfo info);

union FromBytes {
    u8 bytes[2];
    u16 u16;
};

void compile_expr(struct Context *ctx, struct AstNode *node);
void compile_declaration(struct Context *ctx, struct DeclarationNode *node);

void compile_literal(struct Context *ctx, struct LiteralNode *node);
void compile_identifier(struct Context *ctx, struct IdentifierNode *node);
void compile_bin_op(struct Context *ctx, struct BinOpNode *node);

#endif
