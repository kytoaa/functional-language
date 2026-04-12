#include "codegen.h"
#include "../builtins.h"

void compile_declaration(struct Context *ctx, struct DeclarationNode *node)
{
    struct FunctionBindingNode *binding = node->bindings;
    ctx->current_depth += 1;
    while (binding != null) {
        declare_ident(ctx, binding->src_loc);
        binding = binding->next_binding;
    }

    ctx->current_depth -= 1;
}

void compile_expr(struct Context *ctx, struct AstNode *node)
{
    // when function is evaluated it must be in whnf, therefore its argument count should be accessible
    switch (node->kind) {
        case AST_LITERAL:
            compile_literal(ctx, (struct LiteralNode*)node);
            break;
        case AST_IDENTIFIER:
            compile_identifier(ctx, (struct IdentifierNode*)node);
            break;
        case AST_BIN_OP:
            compile_bin_op(ctx, (struct BinOpNode*)node);
            break;
    }
}

void compile_literal(struct Context *ctx, struct LiteralNode *node)
{
    u32 constant = create_constant(ctx, OBJ_BOX, sizeof(struct Value));
    struct Value *value = (struct Value*)get_constant(ctx, constant);

    if (node->type == LITERAL_TYPE_EMPTY_LIST) {
        return;
    }

    switch (node->type) {
        case LITERAL_TYPE_BOOLEAN:{
            value->type = VALUE_BOOL;
            value->as.boolean = node->as.boolean;
            break;
        }
        case LITERAL_TYPE_NUMBER:{
            value->type = VALUE_INT;
            value->as.integer = node->as.number;
            break;
        }
        case LITERAL_TYPE_CHARACTER:{
            value->type = VALUE_CHAR;
            value->as.character = node->as.character;
            break;
        }
        case LITERAL_TYPE_UNIT:{
            value->type = VALUE_UNIT;
            break;
        }
    }
    emit_byte(ctx, OP_PUSH_CONST);
    emit_u16(ctx, constant);
}

void compile_identifier(struct Context *ctx, struct IdentifierNode *node)
{
    u16 ident_offset = get_ident_offset(ctx, node->src_loc).offset;
    // non existent identifier
    if (ident_offset == UINT16_MAX) {
        // TODO: handing code
        return;
    }
    emit_byte(ctx, OP_READ_BINDING);
    emit_u16(ctx, ident_offset);
}

void compile_bin_op(struct Context *ctx, struct BinOpNode *node)
{
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->l);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->r);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    emit_byte(ctx, OP_JUMP_GLOBALS);
    
    u8 op;
    switch (node->op) {
        case AST_BIN_OP_ADD:
            op = GLOBAL_FUNC_ADD;
            break;
        case AST_BIN_OP_SUB:
            op = GLOBAL_FUNC_SUB;
            break;
        case AST_BIN_OP_MUL:
            op = GLOBAL_FUNC_MUL;
            break;
        case AST_BIN_OP_DIV:
            op = GLOBAL_FUNC_DIV;
            break;
        default:
            panic("unreachable, invalid expression");
            break;
    }
    emit_byte(ctx, op);
}

void compile_pattern_match(struct Context *ctx, struct AstNode *node)
{
    if (node->kind == AST_IDENTIFIER) {

    } else if (node->kind == AST_LITERAL) {
        emit_byte(ctx, OP_EVAL);
        compile_literal(ctx, (struct LiteralNode*)node);
        emit_byte(ctx, OP_EQUAL);
    }
}

void compile_case_branch(struct Context *ctx, struct CasePatternNode *node)
{

}

void compile_if_expr(struct Context *ctx, struct IfExprNode *node)
{
    compile_expr(ctx, node->condition);
    emit_byte(ctx, OP_EVAL);
    emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
    emit_byte(ctx, 0);
    u32 true_jump_instruction = get_last_bytecode_index(ctx);
    emit_byte(ctx, 0);

    compile_expr(ctx, node->else_expr);
    emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
    emit_byte(ctx, 0);
    u32 end_jump_instruction = get_last_bytecode_index(ctx);
    emit_byte(ctx, 0);

    u32 true_jump_location = get_last_bytecode_index(ctx) + 1;
    usize diff = true_jump_location - true_jump_instruction;

    union FromBytes diff_bytes = { .u16 = diff };

    u8 *true_jump_instruction_bytes = get_bytecode_byte(ctx, true_jump_instruction);
    true_jump_instruction_bytes[0] = diff_bytes.bytes[0];
    true_jump_instruction_bytes[1] = diff_bytes.bytes[1];

    compile_expr(ctx, node->then_expr);
    u32 end_jump_location = get_last_bytecode_index(ctx) + 1;
    diff_bytes.u16 = end_jump_location - end_jump_instruction;

    u8 *end_jump_instruction_bytes = get_bytecode_byte(ctx, end_jump_instruction);
    end_jump_instruction_bytes[0] = diff_bytes.bytes[0];
    end_jump_instruction_bytes[1] = diff_bytes.bytes[1];
}

void compile_lambda(struct Context *ctx, struct LambdaNode *node)
{
    struct FunctionBindingNode *binding = node->bindings;
    ctx->current_depth += 1;
    u32 bindings = 0;
    while (binding != null) {
        declare_ident(ctx, binding->src_loc);
        bindings += 1;
        binding = binding->next_binding;
    }

    // layout:
    // jump_rel lambda_size
    // lambda_body:
    //   ..lambda body
    // lambda setup

    emit_byte(ctx, OP_JUMP_REL);
    emit_byte(ctx, 0);
    u32 jump_location = get_last_bytecode_index(ctx);
    emit_byte(ctx, 0);

    struct ClosureInfo info = { .arity = bindings, .address = jump_location + 2 };
}

