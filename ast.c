#include "ast.h"

#include <stdlib.h>

static void ast_free_list(AstNode **items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) {
        ast_free(items[i]);
    }
    free(items);
}

static void ast_free_strings(char **items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) {
        free(items[i]);
    }
    free(items);
}

void ast_free(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
        case AST_BLOCK:
            ast_free_list(node->block.items, node->block.count);
            break;
        case AST_EXPR_STMT:
            ast_free(node->expr_stmt.expr);
            break;
        case AST_IF:
            ast_free(node->if_stmt.cond);
            ast_free(node->if_stmt.then_branch);
            ast_free(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->while_stmt.cond);
            ast_free(node->while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->for_stmt.init);
            ast_free(node->for_stmt.cond);
            ast_free(node->for_stmt.step);
            ast_free(node->for_stmt.body);
            break;
        case AST_FOREACH:
            ast_free(node->foreach_stmt.iterable);
            free(node->foreach_stmt.key_name);
            free(node->foreach_stmt.value_name);
            ast_free(node->foreach_stmt.body);
            break;
        case AST_DO_WHILE:
            ast_free(node->do_while_stmt.body);
            ast_free(node->do_while_stmt.cond);
            break;
        case AST_SWITCH:
            ast_free(node->switch_stmt.expr);
            ast_free(node->switch_stmt.strict_expr);
            if (node->switch_stmt.case_exprs) {
                for (int i = 0; i < node->switch_stmt.case_count; i++) {
                    ast_free(node->switch_stmt.case_exprs[i]);
                }
                free(node->switch_stmt.case_exprs);
            }
            if (node->switch_stmt.case_bodies) {
                for (int i = 0; i < node->switch_stmt.case_count; i++) {
                    ast_free(node->switch_stmt.case_bodies[i]);
                }
                free(node->switch_stmt.case_bodies);
            }
            break;
        case AST_GLOBAL:
            ast_free_strings(node->global_stmt.names, node->global_stmt.count);
            break;
        case AST_FUNCTION:
            free(node->func.name);
            if (node->func.params) {
                for (int i = 0; i < node->func.param_count; i++) {
                    free(node->func.params[i]);
                }
                free(node->func.params);
            }
            if (node->func.param_defaults) {
                for (int i = 0; i < node->func.param_count; i++) {
                    ast_free(node->func.param_defaults[i]);
                }
                free(node->func.param_defaults);
            }
            ast_free(node->func.body);
            break;
        case AST_RETURN:
            ast_free(node->ret.value);
            break;
        case AST_BREAK:
        case AST_CONTINUE:
            break;
        case AST_UNSET:
            ast_free(node->unset.target);
            break;
        case AST_INDEX_ASSIGN:
            ast_free(node->index_assign.target);
            ast_free(node->index_assign.value);
            break;
        case AST_ASSIGN:
            free(node->assign.name);
            ast_free(node->assign.value);
            break;
        case AST_ASSIGN_DYNAMIC:
            ast_free(node->assign_dynamic.name_expr);
            ast_free(node->assign_dynamic.value);
            break;
        case AST_INDEX_APPEND:
            ast_free(node->index_append.target);
            break;
        case AST_DESTRUCT_ASSIGN:
            ast_free_list(node->destruct_assign.targets, node->destruct_assign.target_count);
            ast_free(node->destruct_assign.value);
            break;
        case AST_VAR:
            free(node->var.name);
            break;
        case AST_VAR_DYNAMIC:
            ast_free(node->var_dynamic.expr);
            break;
        case AST_BINARY:
            ast_free(node->binary.left);
            ast_free(node->binary.right);
            break;
        case AST_UNARY:
            ast_free(node->unary.expr);
            break;
        case AST_CALL:
            free(node->call.name);
            ast_free_list(node->call.args, node->call.argc);
            break;
        case AST_INDEX:
            ast_free(node->index.target);
            ast_free(node->index.index);
            break;
        case AST_PRE_INC:
        case AST_PRE_DEC:
        case AST_POST_INC:
        case AST_POST_DEC:
            ast_free(node->incdec.target);
            break;
        case AST_ARRAY_LITERAL:
            if (node->array.keys) {
                for (int i = 0; i < node->array.count; i++) {
                    ast_free(node->array.keys[i]);
                }
                free(node->array.keys);
            }
            if (node->array.values) {
                for (int i = 0; i < node->array.count; i++) {
                    ast_free(node->array.values[i]);
                }
                free(node->array.values);
            }
            break;
        case AST_TERNARY:
            ast_free(node->ternary.cond);
            ast_free(node->ternary.then_expr);
            ast_free(node->ternary.else_expr);
            break;
        case AST_NULL_COALESCE:
            ast_free(node->null_coalesce.left);
            ast_free(node->null_coalesce.right);
            break;
        case AST_MAGIC_FUNCTION:
            break;
        case AST_LITERAL:
            switch (node->literal.token.type) {
                case TOK_STRING:
                case TOK_DSTRING:
                case TOK_IDENT:
                case TOK_VAR:
                    free(node->literal.token.string_val);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    free(node);
}
