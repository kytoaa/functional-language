#ifndef func_lang_compiler_expr_h
#define func_lang_compiler_expr_h

#include "codegen.h"

void compile_expr(struct Context *ctx, struct AstNode *node);
void compile_declaration(struct Context *ctx, struct DeclarationNode *node);

void compile_literal(struct Context *ctx, struct LiteralNode *node);
void compile_identifier(struct Context *ctx, struct IdentifierNode *node);
void compile_bin_op(struct Context *ctx, struct BinOpNode *node);

void compile_lambda(struct Context *ctx, struct LambdaNode *node, const char *bind_to);
void compile_thunk(struct Context *ctx, struct AstNode *node, const char *bind_to);

#endif
