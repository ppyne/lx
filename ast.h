/**
 * @file ast.h
 * @brief Abstract syntax tree (AST) node definitions.
 */
#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "lexer.h"

/** AST node kinds. */
typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_EXPR_STMT,

    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_FOREACH,
    AST_DO_WHILE,
    AST_SWITCH,
    AST_GLOBAL,

    AST_FUNCTION,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_UNSET,

    AST_INDEX_ASSIGN,
    AST_ASSIGN,
    AST_ASSIGN_DYNAMIC,
    AST_INDEX_APPEND,
    AST_DESTRUCT_ASSIGN,
    AST_VAR,
    AST_VAR_DYNAMIC,

    AST_BINARY,
    AST_UNARY,

    AST_CALL,
    AST_INDEX,

    AST_PRE_INC,
    AST_PRE_DEC,
    AST_POST_INC,
    AST_POST_DEC,

    AST_ARRAY_LITERAL,

    AST_TERNARY,
    AST_NULL_COALESCE,

    AST_MAGIC_FUNCTION,

    AST_LITERAL
} AstType;

/** Operator kinds for unary/binary/compound assignments. */
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_POW,
    OP_CONCAT,
    OP_ASSIGN,

    OP_EQ, OP_NEQ,
    OP_SEQ, OP_SNEQ,

    OP_LT, OP_LTE, OP_GT, OP_GTE,

    OP_AND, OP_OR,
    OP_NOT,

    OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR,
    OP_BIT_NOT,
    OP_SHL, OP_SHR
} Operator;

/** Forward declaration for AST nodes. */
typedef struct AstNode AstNode;

/**
 * AST node container. The active union field depends on @p type.
 */
struct AstNode {
    AstType type;
    int line;
    int col;
    uint32_t flags;

    union {
        /* program / block */
        struct {
            AstNode **items;
            int count;
        } block;

        /* expression statement */
        struct {
            AstNode *expr;
        } expr_stmt;

        /* if */
        struct {
            AstNode *cond;
            AstNode *then_branch;
            AstNode *else_branch;   /* may be NULL */
        } if_stmt;

        /* while */
        struct {
            AstNode *cond;
            AstNode *body;
        } while_stmt;

        /* do-while */
        struct {
            AstNode *body;
            AstNode *cond;
        } do_while_stmt;

        /* for */
        struct {
            AstNode *init;
            AstNode *cond;
            AstNode *step;
            AstNode *body;
        } for_stmt;

        /* foreach */
        struct {
            AstNode *iterable;
            char *key_name;   /* may be NULL */
            char *value_name;
            AstNode *body;
        } foreach_stmt;

        /* switch */
        struct {
            AstNode *expr;
            AstNode *strict_expr; /* optional */
            AstNode **case_exprs;  /* NULL for default */
            AstNode **case_bodies; /* AST_BLOCK */
            int case_count;
        } switch_stmt;

        /* global */
        struct {
            char **names;
            int count;
        } global_stmt;

        /* function */
        struct {
            char *name;
            char **params;
            AstNode **param_defaults;
            int param_count;
            AstNode *body;
        } func;

        /* return */
        struct {
            AstNode *value;   /* may be NULL */
        } ret;

        /* unset */
        struct {
            AstNode *target;   /* AST_VAR or AST_INDEX */
        } unset;

        /* index assignment */
        struct {
            AstNode *target;   /* AST_INDEX */
            AstNode *value;
            int is_compound;
            Operator op;
        } index_assign;

        /* assignment */
        struct {
            char *name;
            AstNode *value;
            int is_compound;
            Operator op;
        } assign;

        /* dynamic assignment */
        struct {
            AstNode *name_expr;
            AstNode *value;
            int is_compound;
            Operator op;
        } assign_dynamic;

        /* destructuring assignment */
        struct {
            AstNode **targets;
            int target_count;
            AstNode *value;
        } destruct_assign;

        /* variable */
        struct {
            char *name;
        } var;

        /* dynamic variable */
        struct {
            AstNode *expr;
        } var_dynamic;

        /* binary op */
        struct {
            Operator op;
            AstNode *left;
            AstNode *right;
        } binary;

        /* unary op */
        struct {
            Operator op;
            AstNode *expr;
        } unary;

        /* function call */
        struct {
            char *name;
            AstNode **args;
            int argc;
        } call;

        /* index */
        struct {
            AstNode *target;
            AstNode *index;
        } index;

        /* index append */
        struct {
            AstNode *target;
        } index_append;

        /* inc/dec */
        struct {
            AstNode *target;
        } incdec;

        /* array literal */
        struct {
            AstNode **keys;   /* NULL for auto index */
            AstNode **values;
            int count;
        } array;

        /* ternary */
        struct {
            AstNode *cond;
            AstNode *then_expr;
            AstNode *else_expr;
        } ternary;

        /* null coalesce */
        struct {
            AstNode *left;
            AstNode *right;
        } null_coalesce;

        /* literal */
        struct {
            Token token;
        } literal;
    };
};

#define AST_FLAG_HEAP 0x1

void ast_free(AstNode *node);
void ast_register_arena(AstNode *root, void *arena);

#endif /* AST_H */
