#include "expr.h"
#include "codegen.h"
#include "global_resolution.h"
#include "pattern_matching.h"
#include "../builtins.h"

// starts with + to ensure no clashes
static const char *SELF_IDENT = "+closure_self";

void compile_declaration(struct Context *ctx, struct DeclarationNode *node)
{
    declare_ident(ctx, node->name);
    if (node->body->kind == AST_LAMBDA) {
        compile_lambda(ctx, (struct LambdaNode*)node->body, node->name);
        // lambda has been bound, pop it
        emit_byte(ctx, OP_POP_U64);
    } else {
        compile_thunk(ctx, node->body, node->name);
        emit_byte(ctx, OP_POP_U64);
    }
}

void compile_identifier(struct Context *ctx, struct IdentifierNode *node)
{
    struct IdentSearchResult ident = {};
    u8 capture_index = 0;
    u32 global_index = 0;
    if (get_ident_info(ctx, node->src_loc, &ident)) {
        emit_byte(ctx, OP_READ_BINDING);
        emit_u16(ctx, ident.offset);
    } else if (resolve_capture(ctx, node->src_loc, &capture_index)) {
        get_ident_info(ctx, SELF_IDENT, &ident);

        emit_byte(ctx, OP_CAPTURE_READ);
        emit_u16(ctx, ident.offset);
        emit_byte(ctx, capture_index);
    } else {
        enum GlobalResolutionError result = resolve_global(
            ctx->globals,
            ctx->module_index,
            node->src_loc,
            &global_index
        );

        if (result == GLOBAL_RES_OK) {
            emit_byte(ctx, OP_PUSH_CONST);
            emit_u32(ctx, global_index);
        } else {
            non_existent_ident_err(ctx, node->node.loc, null);
        }
    }
}

void compile_literal(struct Context *ctx, struct LiteralNode *node)
{
    u32 constant = 0;
    // unit is at index 0 as all units are identical
    if (node->type != LITERAL_TYPE_UNIT && node->type != LITERAL_TYPE_BOOLEAN) {
        constant = create_constant(ctx->compiling_chunk, OBJ_BOX, sizeof(struct Box));
        struct Value *value = &((struct Box*)get_constant(ctx, constant))->val;

        switch (node->type) {
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
            default:
                panic("unreachable: invalid literal type");
                return;
        }
    } else if (node->type == LITERAL_TYPE_BOOLEAN) {
        // true and false are boxes 1 and 2 respectively
        constant = (node->as.boolean ? 1 : 2) * OBJ_U64_SIZE(struct Box);
    }
    emit_byte(ctx, OP_PUSH_CONST);
    emit_u32(ctx, constant);
}

void compile_bin_op(struct Context *ctx, struct BinOpNode *node)
{
    emit_byte(ctx, OP_PUSH_U64);
    u32 jump_location = get_last_bytecode_index(ctx) + 1;
    emit_u64(ctx, 0);

    compile_expr(ctx, node->r);
    compile_expr(ctx, node->l);

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
        case AST_BIN_OP_CONS:
            op = GLOBAL_FUNC_CONS;
            break;
        case AST_BIN_OP_OR:
            op = GLOBAL_FUNC_OR;
            break;
        case AST_BIN_OP_AND:
            op = GLOBAL_FUNC_AND;
            break;
        case AST_BIN_OP_EQUAL:
            op = GLOBAL_FUNC_EQUAL;
            break;
        case AST_BIN_OP_LESS:
            op = GLOBAL_FUNC_LESS;
            break;
        case AST_BIN_OP_LESS_EQ:
            op = GLOBAL_FUNC_LESS_EQ;
            break;
        case AST_BIN_OP_GREATER:
            op = GLOBAL_FUNC_GREATER;
            break;
        case AST_BIN_OP_GREATER_EQ:
            op = GLOBAL_FUNC_GREATER_EQ;
            break;
        default:
            panic("unreachable, invalid expression");
            break;
    }
    emit_byte(ctx, op);
    u64 end_location = get_last_bytecode_index(ctx) + 1;
    u8 *jump_addr_bytes = get_bytecode_byte(ctx, jump_location);
    for (u8 i = 0; i < 8; i++) {
        jump_addr_bytes[i] = ((u8*)&end_location)[i];
    }
}

void compile_if_expr(struct Context *ctx, struct IfExprNode *node)
{
    compile_expr(ctx, node->condition);
    emit_byte(ctx, OP_EVAL);
    emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
    u32 true_jump_instruction = get_last_bytecode_index(ctx) + 1;
    emit_u16(ctx, 0);

    compile_expr(ctx, node->else_expr);
    emit_byte(ctx, OP_JUMP_REL);
    u32 end_jump_instruction = get_last_bytecode_index(ctx) + 1;
    emit_u16(ctx, 0);

    u32 true_jump_location = get_last_bytecode_index(ctx) + 1;
    i16 diff = true_jump_location - (true_jump_instruction + sizeof(i16));

    u8 *diff_bytes = (u8*)&diff;
    u8 *true_jump_instruction_bytes = get_bytecode_byte(ctx, true_jump_instruction);

    true_jump_instruction_bytes[0] = diff_bytes[0];
    true_jump_instruction_bytes[1] = diff_bytes[1];

    compile_expr(ctx, node->then_expr);
    u32 end_jump_location = get_last_bytecode_index(ctx) + 1;
    diff = end_jump_location - (end_jump_instruction + sizeof(i16));

    diff_bytes = (u8*)&diff;
    u8 *end_jump_instruction_bytes = get_bytecode_byte(ctx, end_jump_instruction);

    end_jump_instruction_bytes[0] = diff_bytes[0];
    end_jump_instruction_bytes[1] = diff_bytes[1];
}

void compile_let_expr(struct Context *ctx, struct LetExprNode *node)
{
    u32 ident_count = ctx->ident_stack_len;

    struct DeclarationNode *decl = (struct DeclarationNode*)node->first_decl;
    while (decl != null) {
        compile_declaration(ctx, decl);
        decl = decl->next_declaration;
    }
    compile_expr(ctx, node->body);
    //compile_thunk(ctx, node->body, null);

    u32 final_ident_count = ctx->ident_stack_len;

    drop_ident(ctx, final_ident_count - ident_count);
    emit_2_bytes(ctx, OP_REMOVE_BINDINGS, final_ident_count - ident_count);
}

/// leaves the constructed lambda at the top of the stack
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
    u32 jump_location = get_last_bytecode_index(&context) + 1;
    emit_u16(ctx, 0);

    // closure expects arguments on stack, first binding at top
    for (u32 i = 0; i < bindings; i++) {
        emit_byte(&context, OP_CREATE_BINDING);
    }
    emit_2_bytes(&context, OP_PUSH_REG_STACK, REG_1);
    emit_byte(&context, OP_CREATE_BINDING);
    declare_ident(&context, SELF_IDENT);

    compile_expr(&context, node->body);

    // bindings + 1 to remove closure pointer
    emit_2_bytes(&context, OP_REMOVE_BINDINGS, bindings + 1);
    // swap result and continuation
    emit_byte(&context, OP_SWAP);
    emit_byte(&context, OP_JUMP);

    u32 jump_target = get_last_bytecode_index(&context) + 1;
    i16 diff = (i32)jump_target - ((i32)jump_location + 2);

    u8 *diff_bytes = (u8*)&diff;
    u8 *jump_location_bytes = get_bytecode_byte(&context, jump_location);

    jump_location_bytes[0] = diff_bytes[0];
    jump_location_bytes[1] = diff_bytes[1];

    struct IdentSearchResult self_ident = {};
    get_ident_info(ctx, SELF_IDENT, &self_ident);

    struct ClosureInfo info = {
        .arity = bindings,
        .address = jump_location + 2,
        .capture_count = context.capture_stack_len,
    };
    u16 closure_info_index = create_closure_info(ctx, info);

    if (context.capture_stack_len > 0) {
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
    } else {
        u32 constant = create_constant(ctx->compiling_chunk, OBJ_CLOSURE, sizeof(struct Closure));
        struct Closure *closure_const = (struct Closure*)get_constant(ctx, constant);
        closure_const->info = (struct ClosureInfo*)(u64)closure_info_index;
        emit_byte(ctx, OP_PUSH_CONST);
        emit_u32(ctx, constant);

        if (bind_to != null) {
            emit_byte(ctx, OP_COPY);
            emit_byte(ctx, OP_CREATE_BINDING);
        }
    }

    end_context(&context);
}

void compile_thunk(struct Context *ctx, struct AstNode *node, const char *bind_to)
{
    struct Context context = {};
    init_context(&context, ctx);

    emit_byte(&context, OP_JUMP_REL);
    u32 jump_location = get_last_bytecode_index(&context) + 1;
    emit_u16(&context, 0);

    emit_2_bytes(&context, OP_PUSH_REG_STACK, REG_1);
    emit_byte(&context, OP_CREATE_BINDING);
    declare_ident(&context, SELF_IDENT);

    compile_expr(&context, node);

    // remove the self binding
    emit_byte(&context, OP_REMOVE_BINDING);

    // swap result and continuation
    emit_byte(&context, OP_SWAP);
    emit_byte(&context, OP_JUMP);


    u32 jump_target = get_last_bytecode_index(&context) + 1;
    i16 diff = (i32)jump_target - ((i32)jump_location + 2);

    u8 *diff_bytes = (u8*)&diff;
    u8 *jump_location_bytes = get_bytecode_byte(&context, jump_location);

    jump_location_bytes[0] = diff_bytes[0];
    jump_location_bytes[1] = diff_bytes[1];

    struct IdentSearchResult self_ident = {};
    get_ident_info(ctx, SELF_IDENT, &self_ident);

    struct ClosureInfo info = {
        .arity = 0,
        .address = jump_location + 2,
        .capture_count = context.capture_stack_len,
    };
    u16 closure_info_index = create_closure_info(ctx, info);

    if (context.capture_stack_len > 0) {
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
    } else {
        u32 constant = create_constant(ctx->compiling_chunk, OBJ_THUNK, sizeof(struct Thunk));
        struct Thunk *thunk_const = (struct Thunk*)get_constant(ctx, constant);
        thunk_const->evaluated = null;
        thunk_const->info = (struct ClosureInfo*)(u64)closure_info_index;
        emit_byte(ctx, OP_PUSH_CONST);
        emit_u32(ctx, constant);

        if (bind_to != null) {
            emit_byte(ctx, OP_COPY);
            emit_byte(ctx, OP_CREATE_BINDING);
        }
    }

    end_context(&context);
}

void namespace_access_expr(struct Context *ctx, struct NamespaceAccessNode *node)
{
    u32 constant_index = 0;
    struct GlobalResolutionResult result = resolve_global_path(
        ctx->globals,
        (struct GlobalSearch){
            .origin_module = ctx->module_index,
            .searching_for = node,
        },
        &constant_index
    );

    if (result.error != GLOBAL_RES_OK) {
        const char *msg = null;
        switch (result.error) {
            case GLOBAL_RES_ERROR_PRIVATE:
                msg = "identifier is private";
                break;
            case GLOBAL_RES_ERROR_ROOT_SUPER:
                msg = "tried to get 'super' of the root module";
                break;
            case GLOBAL_RES_ERROR_RECURSION_LIMIT:
                msg = "reached recursion limit trying to evaluate path";
                break;
            default:
                break;
        }
        non_existent_ident_err(ctx, result.error_finding->node.loc, msg);
        return;
    }

    emit_byte(ctx, OP_PUSH_CONST);
    emit_u32(ctx, constant_index);
}

void compile_application(struct Context *ctx, struct ApplicationNode *application)
{
    struct ApplicationNode *current_appl = application;
    u32 args = 0;

    for (;;) {
        compile_expr(ctx, current_appl->argument);
        args += 1;

        if (current_appl->function->kind != AST_APPLICATION)
            break;

        current_appl = (struct ApplicationNode*)current_appl->function;
    }
    compile_expr(ctx, current_appl->function);

    emit_byte(ctx, OP_EVAL);
    emit_2_bytes(ctx, OP_PARTIAL_APPLY, args);
}

void compile_expr(struct Context *ctx, struct AstNode *node)
{
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
        case AST_APPLICATION:
            compile_application(ctx, (struct ApplicationNode*)node);
            break;
        case AST_CASE_EXPR:
            compile_case_expression(ctx, (struct CaseExprNode*)node);
            break;
        case AST_IF_EXPR:
            compile_if_expr(ctx, (struct IfExprNode*)node);
            break;
        case AST_UNDERSCORE:
            used_underscore_err(ctx, node->loc, null);
            break;
        case AST_NAMESPACE_ACCESS:
            namespace_access_expr(ctx, (struct NamespaceAccessNode*)node);
            break;
        default:
            panic("unknown ast node");
            break;
    }
}
