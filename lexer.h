/**
 * @file lexer.h
 * @brief Token definitions and lexer interface.
 */
#ifndef LEXER_H
#define LEXER_H

/** Token kinds produced by the lexer. */
typedef enum {
    TOK_EOF,
    TOK_ERROR,

    // literals
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_DSTRING,
    TOK_ARRAY,

    // identifiers
    TOK_IDENT,
    TOK_VAR,

    // keywords
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_FOREACH,
    TOK_DO,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_FUNCTION,
    TOK_RETURN,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_UNSET,
    TOK_AS,

    TOK_NULL,
    TOK_UNDEFINED,
    TOK_VOID,
    TOK_TRUE,
    TOK_FALSE,

    // operators
    TOK_ASSIGN,     // =
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MUL,        // *
    TOK_DIV,        // /
    TOK_MOD,        // %
    TOK_POW,        // **
    TOK_DOT,        // .
    TOK_PLUS_PLUS,  // ++
    TOK_MINUS_MINUS, // --
    TOK_PLUS_EQ,    // +=
    TOK_MINUS_EQ,   // -=
    TOK_MUL_EQ,     // *=
    TOK_DIV_EQ,     // /=
    TOK_DOT_EQ,     // .=

    TOK_EQ,         // ==
    TOK_NEQ,        // !=
    TOK_SEQ,        // ===
    TOK_SNEQ,       // !==

    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,

    TOK_AND,        // &&
    TOK_OR,         // ||
    TOK_NOT,        // !
    TOK_DOLLAR,     // $

    TOK_BIT_AND,    // &
    TOK_BIT_OR,     // |
    TOK_BIT_XOR,    // ^
    TOK_BIT_NOT,    // ~
    TOK_SHL,        // <<
    TOK_SHR,        // >>

    // delimiters
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_SEMI,
    TOK_QUESTION,   // ?
    TOK_COLON,      // :
    TOK_ARROW       // =>
} TokenType;

/** Token payload with optional literal data. */
typedef struct {
    TokenType type;
    int line;
    int col;

    union {
        int    int_val;
        double float_val;
        char  *string_val;
    };
} Token;

/** Lexer state. */
typedef struct {
    const char *src;
    const char *cur;
    const char *filename;
    int line;
    int col;
    int start_line;
    int start_col;
} Lexer;

/** Initialize a lexer over @p source. */
void  lexer_init(Lexer *lx, const char *source, const char *filename);
/** Return the next token from @p lx. */
Token lexer_next(Lexer *lx);

#endif
