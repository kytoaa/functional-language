#include "codegen.h"
#include "expr.h"
#include "../builtins.h"
#include "global_resolution.h"

struct CaseBranchResult {
    u32 failure_index;
    u32 success_index;
};

struct CaseBranchResult compile_pattern_match_branch(struct Context *ctx, struct CasePatternNode *node);

void compile_case_expression(struct Context *ctx, struct CaseExprNode *node)
{
    compile_expr(ctx, node->value);

    emit_byte(ctx, OP_JUMP_REL);
    emit_u16(ctx, 3);
    emit_byte(ctx, OP_JUMP_REL);
    u32 success_jump_index = get_last_bytecode_index(ctx) + 1;
    emit_u16(ctx, 0);
    
    struct CasePatternNode *branch = (struct CasePatternNode*)node->first_pattern;

    // set up stack with [..bindings, expr]
    emit_2_bytes(ctx, OP_PUSH_REG_STACK, BINDING_PTR);
    emit_byte(ctx, OP_SWAP);

    while (branch != null) {
        emit_byte(ctx, OP_SWAP);
        // unbind any extra bindings
        emit_2_bytes(ctx, OP_COPY_STACK_REG, BINDING_PTR);
        // set up stack with [..bindings, expr, expr]
        emit_byte(ctx, OP_SWAP);
        emit_byte(ctx, OP_COPY);

        struct CaseBranchResult result = compile_pattern_match_branch(ctx, branch);
        i16 diff = (i32)(success_jump_index - 1) - (i32)(result.success_index + 2);
        u8 *diff_bytes = (u8*)&diff;
        u8 *success_jump_bytes = get_bytecode_byte(ctx, result.success_index);
        
        success_jump_bytes[0] = diff_bytes[0];
        success_jump_bytes[1] = diff_bytes[1];

        if (result.failure_index != 0) {
            i16 failure_diff = (i32)(get_last_bytecode_index(ctx) + 1) - (i32)(result.failure_index + 2);
            u8 *failure_diff_bytes = (u8*)&failure_diff;
            u8 *failure_jump_bytes = get_bytecode_byte(ctx, result.failure_index);

            failure_jump_bytes[0] = failure_diff_bytes[0];
            failure_jump_bytes[1] = failure_diff_bytes[1];
        }

        branch = branch->next_pattern;
    }
    emit_byte(ctx, OP_PUSH_U64);
    emit_u64(ctx, (u64)"could not match any pattern\n");
    emit_byte(ctx, OP_CALL_EXTERN);
    emit_byte(ctx, VM_EXTERN_FUNC_STDERR);
    emit_byte(ctx, OP_CALL_EXTERN);
    emit_byte(ctx, VM_EXTERN_FUNC_WRITE_C_STRING);
    emit_2_bytes(ctx, OP_JUMP_GLOBALS, GLOBAL_FUNC_ERROR);
    u32 success_end_addr = get_last_bytecode_index(ctx) + 1;
    i16 success_diff = (i32)success_end_addr - (i32)(success_jump_index + 2);
    u8 *success_diff_bytes = (u8*)&success_diff;
    u8 *success_jump_bytes = get_bytecode_byte(ctx, success_jump_index);

    success_jump_bytes[0] = success_diff_bytes[0];
    success_jump_bytes[1] = success_diff_bytes[1];
}

static u8 type_check_op(enum LiteralType lit)
{
    switch (lit) {
        case LITERAL_TYPE_NUMBER:
            return OP_IS_INT;
        case LITERAL_TYPE_BOOLEAN:
            return OP_IS_BOOL;
        case LITERAL_TYPE_UNIT:
            return OP_IS_UNIT;
        case LITERAL_TYPE_CHARACTER:
            return OP_IS_CHAR;
    }
}

/// expects expression to match at top of stack, returns index of jump on match failure
/// returns 0 if irrefutable
static u32 compile_pattern_node(struct Context *ctx, struct AstNode *node)
{
    switch (node->kind) {
        case AST_IDENTIFIER:{
            struct IdentifierNode *ident = (struct IdentifierNode*)node;
            declare_ident(ctx, ident->src_loc);
            emit_byte(ctx, OP_CREATE_BINDING);
            return 0;
        }
        case AST_UNDERSCORE:{
            emit_byte(ctx, OP_POP_U64);
            return 0;
        }
        case AST_BIN_OP:{
            struct BinOpNode *cons = (struct BinOpNode*)node;
            emit_byte(ctx, OP_EVAL);
            emit_byte(ctx, OP_IS_CONS);
            emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
            emit_u16(ctx, 4);
            emit_byte(ctx, OP_POP_U64);
            emit_byte(ctx, OP_JUMP_REL);
            u32 cons_check_index = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, 0);

            emit_byte(ctx, OP_COPY);
            emit_byte(ctx, OP_HEAD);
            u32 l_failure = compile_pattern_node(ctx, cons->l);

            emit_byte(ctx, OP_TAIL);
            u32 r_failure = compile_pattern_node(ctx, cons->r);

            if (l_failure != 0) {
                // l should jump to `POP_U64` as the cons is still below on the stack
                i16 l_jump = (i32)(cons_check_index - 2) - (i32)(l_failure + 2);
                u8 *l_failure_bytes = get_bytecode_byte(ctx, l_failure);
                u8 *l_jump_bytes = (u8*)&l_jump;
                l_failure_bytes[0] = l_jump_bytes[0];
                l_failure_bytes[1] = l_jump_bytes[1];
            }
            if (r_failure != 0) {
                i16 r_jump = (i32)(cons_check_index - 1) - (i32)(r_failure + 2);
                u8 *r_failure_bytes = get_bytecode_byte(ctx, r_failure);
                u8 *r_jump_bytes = (u8*)&r_jump;
                r_failure_bytes[0] = r_jump_bytes[0];
                r_failure_bytes[1] = r_jump_bytes[1];
            }
            return cons_check_index;
        }
        case AST_NAMESPACE_ACCESS:
        case AST_APPLICATION:{
            struct ApplicationNode *appl = (struct ApplicationNode*)node;
            emit_byte(ctx, OP_EVAL);
            emit_byte(ctx, OP_IS_OBJ);
            emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
            emit_u16(ctx, 4);
            emit_byte(ctx, OP_POP_U64);
            emit_byte(ctx, OP_JUMP_REL);
            u32 failure_index = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, 0);

            u16 arg_count = 0;
            struct AstNode *constructor_ident = node;
            if (node->kind == AST_APPLICATION) {
                struct ApplicationNode *appl_node = appl;
                while (appl_node != null) {
                    arg_count += 1;

                    if (appl_node->function->kind != AST_APPLICATION)
                        break;

                    appl_node = (struct ApplicationNode*)appl_node->function;
                }
                constructor_ident = appl_node->function;
            }

            u16 variant = 0;

            if (constructor_ident->kind == AST_IDENTIFIER) {
                enum GlobalResolutionError error = resolve_global(
                    ctx->globals,
                    ctx->module_index,
                    ((struct IdentifierNode*)constructor_ident)->src_loc,
                    null,
                    &variant
                );
                if (error != GLOBAL_RES_OK) {
                    non_existent_ident_err(ctx, constructor_ident->loc, null);
                    return 0;
                }
            } else if (constructor_ident->kind == AST_NAMESPACE_ACCESS) {
                struct GlobalResolutionResult result = resolve_global_path(
                    ctx->globals,
                    (struct GlobalSearch){
                        .origin_module = ctx->module_index,
                        .searching_for = (struct NamespaceAccessNode*)constructor_ident,
                    },
                    null,
                    &variant
                );
                if (result.error != GLOBAL_RES_OK) {
                    namespace_access_error(ctx, result);
                    return 0;
                }
            } else {
                invalid_pattern_err(ctx, constructor_ident->loc, "not a constructor");
                return 0;
            }

            emit_byte(ctx, OP_IS_VARIANT);
            u32 variant_index = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, variant);

            emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
            emit_u16(ctx, 3);
            emit_byte(ctx, OP_JUMP_REL);
            u32 current_index = get_last_bytecode_index(ctx) + 1;
            i16 jump = (i32)(failure_index - 2) - (i32)(current_index + 2);
            emit_u16(ctx, jump);

            if (variant == (u16)-1) {
                if (constructor_ident->kind == AST_IDENTIFIER) {
                    enqueue_remapping_work(ctx->remapping_queue, (struct RemappingWork){
                        .identifier = (struct IdentifierNode*)constructor_ident,
                        .loc = constructor_ident->loc,
                        .arg_count = arg_count,
                        .bytecode_index = variant_index,
                        .searching_from_module = ctx->module_index,
                        .is_namespace = false,
                        .is_pattern_constructor = true,
                    });
                } else {
                    struct AstNode *namespace = constructor_ident;
                    while (namespace->kind == AST_NAMESPACE_ACCESS) {
                        namespace = ((struct NamespaceAccessNode*)namespace)->rhs;
                    }
                    enqueue_remapping_work(ctx->remapping_queue, (struct RemappingWork){
                        .namespace_access = (struct NamespaceAccessNode*)constructor_ident,
                        .loc = namespace->loc,
                        .arg_count = arg_count,
                        .bytecode_index = variant_index,
                        .searching_from_module = ctx->module_index,
                        .is_namespace = true,
                        .is_pattern_constructor = true,
                    });
                }
            }

            if (arg_count > 0) {
                struct ApplicationNode *current_appl = appl;
                u8 argument = arg_count - 1;
                while (current_appl != null) {
                    emit_2_bytes(ctx, OP_OBJECT_READ, argument);
                    u32 failure = compile_pattern_node(ctx, current_appl->argument);

                    if (failure != 0) {
                        // should jump to `POP_U64` as the object is still below on the stack
                        i16 jump = (i32)(failure_index - 2) - (i32)(failure + 2);
                        u8 *failure_bytes = get_bytecode_byte(ctx, failure);
                        u8 *jump_bytes = (u8*)&jump;
                        failure_bytes[0] = jump_bytes[0];
                        failure_bytes[1] = jump_bytes[1];
                    }
                    argument -= 1;
                    if (current_appl->function->kind != AST_APPLICATION)
                        break;

                    current_appl = (struct ApplicationNode*)current_appl->function;
                }
            }

            emit_byte(ctx, OP_POP_U64);

            return failure_index;
        }
        case AST_LITERAL:{
            struct LiteralNode *literal = (struct LiteralNode*)node;

            emit_byte(ctx, OP_EVAL);
            emit_byte(ctx, type_check_op(literal->type));
            emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
            emit_u16(ctx, 4);
            emit_byte(ctx, OP_POP_U64);
            emit_byte(ctx, OP_JUMP_REL);
            u32 failure_index = get_last_bytecode_index(ctx) + 1;
            emit_u16(ctx, 0);

            compile_literal(ctx, literal);
            emit_byte(ctx, OP_EVAL);
            emit_byte(ctx, OP_EQUAL);

            emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
            emit_u16(ctx, 3);
            emit_byte(ctx, OP_JUMP_REL);
            u32 current_index = get_last_bytecode_index(ctx) + 1;
            i16 jump = (i32)(failure_index - 1) - (i32)(current_index + 2);
            emit_u16(ctx, jump);

            return failure_index;
        }
        default:
            invalid_pattern_err(ctx, node->loc, null);
            return 0;
    }
}

/// returns the bytecode index of the pattern exit
struct CaseBranchResult compile_pattern_match_branch(struct Context *ctx, struct CasePatternNode *node)
{
    u32 ident_count = ctx->ident_stack_len;

    u32 pattern_failure_index = compile_pattern_node(ctx, node->pattern);

    u32 exit_point_index = pattern_failure_index;

    if (node->condition != null) {
        compile_expr(ctx, node->condition);
        emit_byte(ctx, OP_JUMP_REL_CONDITIONAL);
        emit_u16(ctx, 3);
        emit_byte(ctx, OP_JUMP_REL);
        exit_point_index = get_last_bytecode_index(ctx) + 1;
        emit_u16(ctx, 0);

        if (pattern_failure_index != 0) {
            i16 diff = (i32)(exit_point_index - 1) - (i32)(pattern_failure_index + 2);
            u8 *diff_bytes = (u8*)&diff;
            u8 *pattern_failure_bytes = get_bytecode_byte(ctx, pattern_failure_index);
            pattern_failure_bytes[0] = diff_bytes[0];
            pattern_failure_bytes[1] = diff_bytes[1];
        }
    }

    emit_byte(ctx, OP_POP_U64);
    emit_byte(ctx, OP_POP_U64);
    compile_expr(ctx, node->body);

    u32 final_ident_count = ctx->ident_stack_len;

    drop_ident(ctx, final_ident_count - ident_count);
    emit_2_bytes(ctx, OP_REMOVE_BINDINGS, final_ident_count - ident_count);

    emit_byte(ctx, OP_JUMP_REL);
    u32 correct_index = get_last_bytecode_index(ctx) + 1;
    emit_u16(ctx, 0);

    return (struct CaseBranchResult){
        .failure_index = exit_point_index,
        .success_index = correct_index,
    };
}

