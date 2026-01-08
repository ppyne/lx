/**
 * @file parser.h
 * @brief Parser interface for building the AST.
 */
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/** Parser state and lookahead tokens. */
typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
} Parser;

/** Parse a full program into an AST. */
AstNode *parse_program(Parser *p);

#endif
