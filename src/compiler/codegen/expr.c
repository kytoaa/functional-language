#include "codegen.h"
#include "../builtins.h"

// starts with + to ensure no clashes
static const char *SELF_IDENT = "+closure_self";

void compile_lambda(struct Context *ctx, struct LambdaNode *node, const char *bind_to);
void compile_thunk(struct Context *ctx, struct AstNode *node, const char *bind_to);

void compile_declaration(struct Context *ctx, struct DeclarationNode *node)
{
    declare_ident(ctx, node->name);
    if (node->body->kind == AST_LAMBDA) {
        compile_lambda(ctx, (struct LambdaNode*)node->body, node->name);
    } else {
        compile_thunk(ctx, node->body, node->name);
    }
}

void compile_identifier(struct Context *ctx, struct IdentifierNode *node)
{
    struct IdentSearchResult ident = {};
    u8 capture_index = 0;
    if (get_ident_info(ctx, node->src_loc, &ident)) {
        emit_byte(ctx, OP_READ_BINDING);
        emit_u16(ctx, ident.offset);
    } else if (resolve_capture(ctx, node->src_loc, &capture_index)) {
        get_ident_info(ctx, SELF_IDENT, &ident);

        emit_byte(ctx, OP_CAPTURE_READ);
        emit_u16(ctx, ident.offset);
        emit_byte(ctx, capture_index);
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

void compile_bin_op(struct Context *ctx, struct BinOpNode *node)
{
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->l);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    compile_expr(ctx, node->r);
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, INSTRUCTION_PTR);
    emit_byte(ctx, OP_JUMP_GLOBALS);
    
    u8 op = GLOBAL_FUNC_ADD;
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

void compile_let_expr(struct Context *ctx, struct LetExprNode *node)
{
    struct DeclarationNode *decl = (struct DeclarationNode*)node->first_decl;
    while (decl != null) {
        compile_declaration(ctx, decl);
        decl = decl->next_declaration;
    }
    compile_thunk(ctx, node->body, null);
}

void compile_lambda(struct Context *ctx, struct LambdaNode *node, const char *bind_to)
{
    struct Context context = {};
    init_context(&context, ctx);

    struct FunctionBindingNode *binding = node->bindings;
    u32 bindings = 0;
    while (binding != null) {
        declare_ident(&context, binding->src_loc);
        bindings += 1;
        binding = binding->next_binding;
    }

    // layout:
    // jump_rel lambda_size
    // lambda_body:
    //   ..lambda body
    // lambda setup

    emit_byte(&context, OP_JUMP_REL);
    emit_byte(&context, 0);
    u32 jump_location = get_last_bytecode_index(&context);
    emit_byte(&context, 0);

    // closure expects arguments on stack, first binding at top
    for (u32 i = 0; i < bindings; i++) {
        emit_byte(&context, OP_CREATE_BINDING);
    }
    emit_2_bytes(&context, OP_PUSH_REG_STACK, REG_1);
    emit_byte(&context, OP_CREATE_BINDING);
    declare_ident(&context, SELF_IDENT);

    compile_expr(&context, node->body);
    // bindings + 1 to remove closure pointer
    for (u32 i = 0; i < bindings + 1; i++) {
        emit_byte(&context, OP_REMOVE_BINDING);
    }
    emit_byte(&context, OP_JUMP);
    u32 jump_target = get_last_bytecode_index(&context) + 1;
    union FromBytes diff_bytes = { .u16 = (u16)(jump_target - jump_location) };
    u8 *jump_location_bytes = get_bytecode_byte(&context, jump_location);
    jump_location_bytes[0] = diff_bytes.bytes[0];
    jump_location_bytes[1] = diff_bytes.bytes[1];

    struct IdentSearchResult self_ident = {};
    get_ident_info(ctx, SELF_IDENT, &self_ident);

    struct ClosureInfo info = {
        .arity = bindings,
        .address = jump_location + 2,
        .capture_count = context.capture_stack_len,
    };
    u16 closure_info_index = create_closure_info(ctx, info);

    if (bind_to != null) {
        // no bindings have been added, if `bind_to != null` then the caller
        // already created a binding in `ctx` which gets bound here
        emit_byte(ctx, OP_CREATE_CLOSURE);
        emit_u16(ctx, closure_info_index);
        emit_byte(ctx, OP_CREATE_BINDING);
    }

    for (u32 i = 0; i < context.capture_stack_len; i++) {
        struct Capture capture = context.capture_stack[i];
        if (capture.is_local) {
            emit_byte(ctx, OP_READ_BINDING);
            emit_u16(ctx, capture.parent_index);
        } else {
            emit_byte(ctx, OP_CAPTURE_READ);
            emit_u16(ctx, self_ident.offset);
            emit_byte(ctx, capture.parent_index);
        }
    }

    if (bind_to == null) {
        emit_byte(ctx, OP_CREATE_CLOSURE);
        emit_u16(ctx, closure_info_index);
    } else {
        get_ident_info(ctx, bind_to, &self_ident);
        emit_byte(ctx, OP_READ_BINDING);
        emit_u16(ctx, self_ident.offset);
    }
    emit_byte(ctx, OP_WRITE_CLOSURE);

    end_context(&context);
}

void compile_thunk(struct Context *ctx, struct AstNode *node, const char *bind_to)
{
    struct Context context = {};
    init_context(&context, ctx);

    emit_byte(&context, OP_JUMP_REL);
    emit_byte(&context, 0);
    u32 jump_location = get_last_bytecode_index(&context);
    emit_byte(&context, 0);

    emit_2_bytes(&context, OP_PUSH_REG_STACK, REG_1);
    emit_byte(&context, OP_CREATE_BINDING);
    declare_ident(&context, SELF_IDENT);

    compile_expr(&context, node);

    emit_byte(&context, OP_JUMP);
    u32 jump_target = get_last_bytecode_index(&context) + 1;
    union FromBytes diff_bytes = { .u16 = (u16)(jump_target - jump_location) };
    u8 *jump_location_bytes = get_bytecode_byte(&context, jump_location);
    jump_location_bytes[0] = diff_bytes.bytes[0];
    jump_location_bytes[1] = diff_bytes.bytes[1];

    struct IdentSearchResult self_ident = {};
    get_ident_info(ctx, SELF_IDENT, &self_ident);

    struct ClosureInfo info = {
        .arity = 0,
        .address = jump_location + 2,
        .capture_count = context.capture_stack_len,
    };
    u16 closure_info_index = create_closure_info(ctx, info);

    if (bind_to != null) {
        // no bindings have been added, if `bind_to != null` then the caller
        // already created a binding in `ctx` which gets bound here
        emit_byte(ctx, OP_CREATE_THUNK);
        emit_u16(ctx, closure_info_index);
        emit_byte(ctx, OP_CREATE_BINDING);
    }

    for (u32 i = 0; i < context.capture_stack_len; i++) {
        struct Capture capture = context.capture_stack[i];
        if (capture.is_local) {
            emit_byte(ctx, OP_READ_BINDING);
            emit_u16(ctx, capture.parent_index);
        } else {
            emit_byte(ctx, OP_CAPTURE_READ);
            emit_u16(ctx, self_ident.offset);
            emit_byte(ctx, capture.parent_index);
        }
    }

    if (bind_to == null) {
        emit_byte(ctx, OP_CREATE_THUNK);
        emit_u16(ctx, closure_info_index);
    } else {
        get_ident_info(ctx, bind_to, &self_ident);
        emit_byte(ctx, OP_READ_BINDING);
        emit_u16(ctx, self_ident.offset);
    }
    emit_byte(ctx, OP_WRITE_THUNK);

    end_context(&context);
}


void compile_expr(struct Context *ctx, struct AstNode *node)
{
    // when function is evaluated it must be in whnf, therefore its argument count should be accessible
    switch (node->kind) {
        case AST_LITERAL:
            compile_literal(ctx, (struct LiteralNode*)node);
            break;
        case AST_LAMBDA:
            compile_lambda(ctx, (struct LambdaNode*)node, null);
            break;
        case AST_IDENTIFIER:
            compile_identifier(ctx, (struct IdentifierNode*)node);
            break;
        case AST_BIN_OP:
            compile_bin_op(ctx, (struct BinOpNode*)node);
            break;
        case AST_LET_EXPR:
            compile_let_expr(ctx, (struct LetExprNode*)node);
            break;
    }
}
