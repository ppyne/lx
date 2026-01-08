/**
 * @file eval.h
 * @brief AST evaluation interface.
 */
#ifndef EVAL_H
#define EVAL_H

#include "env.h"
#include "natives.h"
#include "ast.h"

/** Control-flow signals returned by evaluation. */
typedef enum {
    FLOW_NORMAL = 0,
    FLOW_RETURN,
    FLOW_BREAK,
    FLOW_CONTINUE
} EvalFlow;

/** Evaluation result with optional control-flow. */
typedef struct {
    EvalFlow flow;
    Value value;
} EvalResult;

/** Evaluate a node and return the result plus control-flow signal. */
EvalResult eval_node(AstNode *n, Env *env);

/** Execute a program or block node. */
EvalResult eval_program(AstNode *program, Env *env);

#endif
