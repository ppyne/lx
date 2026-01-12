/**
 * @file lexer.c
 * @brief Lexer implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <stdint.h>

#include "lx_version.h"
#include "lexer.h"

static char peek(Lexer *l) {
    return *l->cur;
}

static char peek_next(Lexer *l) {
    if (*l->cur == 0) return 0;
    return l->cur[1];
}

static char advance(Lexer *l) {
    char c = *l->cur;
    if (!c) return 0;
    l->cur++;
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static int match(Lexer *l, char c) {
    if (*l->cur != c) return 0;
    l->cur++;
    return 1;
}

static int lx_endianness(void) {
    union { uint16_t u; unsigned char b[2]; } u = {1};
    return (u.b[0] == 1) ? 0 : 1;
}

static Token make_token(Lexer *l, TokenType t) {
    Token tok;
    tok.type = t;
    tok.line = l->start_line;
    tok.col = l->start_col;
    return tok;
}

static Token error_token(Lexer *l) {
    return make_token(l, TOK_ERROR);
}

static int append_char(char **buf, size_t *cap, size_t *len, char c) {
    if (*cap <= *len) {
        size_t next = (*cap == 0) ? 64 : (*cap * 2);
        char *nb = (char *)realloc(*buf, next);
        if (!nb) return 0;
        *buf = nb;
        *cap = next;
    }
    (*buf)[(*len)++] = c;
    return 1;
}

static void skip_spaces(Lexer *l) {
    for (;;) {
        char c = peek(l);

        // Whitespace
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(l);
            continue;
        }

        // New line
        if (c == '\n') {
            advance(l);
            continue;
        }

        // Line comment //
        if (c == '/' && peek_next(l) == '/') {
            while (peek(l) && peek(l) != '\n')
                advance(l);
            continue;
        }

        // Line comment #
        if (c == '#') {
            while (peek(l) && peek(l) != '\n')
                advance(l);
            continue;
        }

        // Block comment /* */
        if (c == '/' && peek_next(l) == '*') {
            advance(l); // /
            advance(l); // *
            while (peek(l)) {
                if (peek(l) == '*' && peek_next(l) == '/') {
                    advance(l); // *
                    advance(l); // /
                    break;
                }
                advance(l);
            }
            continue;
        }

        break;
    }
}

static Token dstring(Lexer *l) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    while (peek(l) && peek(l) != '"') {
        char c = advance(l);
        if (c == '\\' && peek(l)) {
            char n = advance(l);
            if (!append_char(&buf, &cap, &len, '\\')) return error_token(l);
            if (!append_char(&buf, &cap, &len, n)) return error_token(l);
        } else {
            if (!append_char(&buf, &cap, &len, c)) return error_token(l);
        }
    }

    if (!match(l, '"')) {
        free(buf);
        return error_token(l);
    }

    if (!append_char(&buf, &cap, &len, '\0')) {
        free(buf);
        return error_token(l);
    }

    Token tok = make_token(l, TOK_DSTRING);
    tok.string_val = buf;
    return tok;
}

static Token sstring(Lexer *l) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    while (peek(l) && peek(l) != '\'') {
        char c = advance(l);
        if (c == '\\') {
            char n = peek(l);
            if (n == '\\' || n == '\'') {
                advance(l);
                if (!append_char(&buf, &cap, &len, n)) return error_token(l);
            } else {
                if (!append_char(&buf, &cap, &len, c)) return error_token(l);
            }
        } else {
            if (!append_char(&buf, &cap, &len, c)) return error_token(l);
        }
    }

    if (!match(l, '\'')) {
        free(buf);
        return error_token(l);
    }

    if (!append_char(&buf, &cap, &len, '\0')) {
        free(buf);
        return error_token(l);
    }

    Token tok = make_token(l, TOK_STRING);
    tok.string_val = buf;
    return tok;
}

static Token number(Lexer *l) {
    const char *start = l->cur;
    int is_float = 0;

    if (peek(l) == '.') {
        is_float = 1;
        advance(l);
        while (isdigit(peek(l))) advance(l);
    } else {
        if (peek(l) == '0') {
            advance(l);
            if (peek(l) == 'x' || peek(l) == 'X') {
                advance(l);
                while (isxdigit(peek(l))) advance(l);
    Token tok = make_token(l, TOK_INT);
    tok.int_val = (lx_int_t)strtoll(start, NULL, 16);
                return tok;
            }
            if (peek(l) == 'b' || peek(l) == 'B') {
                advance(l);
                while (peek(l) == '0' || peek(l) == '1') advance(l);
                Token tok = make_token(l, TOK_INT);
            tok.int_val = (lx_int_t)strtoll(start + 2, NULL, 2);
                return tok;
            }
            while (isdigit(peek(l))) advance(l);
        } else {
            while (isdigit(peek(l))) advance(l);
        }

        if (peek(l) == '.') {
            is_float = 1;
            advance(l);
            while (isdigit(peek(l))) advance(l);
        }
    }

    if (peek(l) == 'e' || peek(l) == 'E') {
        is_float = 1;
        advance(l);
        if (peek(l) == '+' || peek(l) == '-') advance(l);
        while (isdigit(peek(l))) advance(l);
    }

    if (is_float) {
        Token tok = make_token(l, TOK_FLOAT);
        tok.float_val = strtod(start, NULL);
        return tok;
    }

    if (start[0] == '0' && isdigit((unsigned char)start[1]) && start[1] != '8' && start[1] != '9') {
        int octal_ok = 1;
        for (const char *p = start + 1; p < l->cur; p++) {
            if (*p < '0' || *p > '7') { octal_ok = 0; break; }
        }
        if (octal_ok) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = (lx_int_t)strtoll(start, NULL, 8);
            return tok;
        }
    }

    Token tok = make_token(l, TOK_INT);
    tok.int_val = (lx_int_t)strtoll(start, NULL, 10);
    return tok;
}

static Token identifier(Lexer *l, int is_var) {
    const char *start = l->cur;
    while (isalnum(peek(l)) || peek(l) == '_')
        advance(l);

    int len = l->cur - start;
    const char *s = start;

    #define KW(name, tok) if (len == strlen(name) && !strncmp(s, name, len)) return make_token(l, tok)

    if (!is_var) {
        if (len == 6 && !strncmp(s, "LX_EOL", len)) {
            Token tok = make_token(l, TOK_STRING);
            tok.string_val = strdup("\n");
            return tok;
        }
        if (len == 10 && !strncmp(s, "LX_VERSION", len)) {
            Token tok = make_token(l, TOK_STRING);
            tok.string_val = strdup(LX_VERSION_STRING);
            return tok;
        }
        if (len == 10 && !strncmp(s, "LX_INT_MAX", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = (lx_int_t)LX_INT_MAX;
            return tok;
        }
        if (len == 10 && !strncmp(s, "LX_INT_MIN", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = (lx_int_t)LX_INT_MIN;
            return tok;
        }
        if (len == 11 && !strncmp(s, "LX_INT_SIZE", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = (lx_int_t)LX_INT_SIZE;
            return tok;
        }
        if (len == 13 && !strncmp(s, "LX_ENDIANNESS", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = lx_endianness();
            return tok;
        }
        if (len == 12 && !strncmp(s, "LX_FLOAT_DIG", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = DBL_DIG;
            return tok;
        }
        if (len == 13 && !strncmp(s, "LX_FLOAT_SIZE", len)) {
            Token tok = make_token(l, TOK_INT);
            tok.int_val = (int)sizeof(double);
            return tok;
        }
        if (len == 16 && !strncmp(s, "LX_FLOAT_EPSILON", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = DBL_EPSILON;
            return tok;
        }
        if (len == 12 && !strncmp(s, "LX_FLOAT_MIN", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = DBL_MIN;
            return tok;
        }
        if (len == 12 && !strncmp(s, "LX_FLOAT_MAX", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = DBL_MAX;
            return tok;
        }
        if (len == 3 && !strncmp(s, "M_E", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 2.71828182845904523536;
            return tok;
        }
        if (len == 4 && !strncmp(s, "M_PI", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 3.14159265358979323846;
            return tok;
        }
        if (len == 5 && !strncmp(s, "M_LN2", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 0.69314718055994530942;
            return tok;
        }
        if (len == 6 && !strncmp(s, "M_LN10", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 2.30258509299404568402;
            return tok;
        }
        if (len == 7 && !strncmp(s, "M_LOG2E", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 1.44269504088896340736;
            return tok;
        }
        if (len == 8 && !strncmp(s, "M_LOG10E", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 0.43429448190325182765;
            return tok;
        }
        if (len == 7 && !strncmp(s, "M_SQRT2", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 1.41421356237309504880;
            return tok;
        }
        if (len == 9 && !strncmp(s, "M_SQRT1_2", len)) {
            Token tok = make_token(l, TOK_FLOAT);
            tok.float_val = 0.70710678118654752440;
            return tok;
        }
        KW("if", TOK_IF);
        KW("else", TOK_ELSE);
        KW("while", TOK_WHILE);
        KW("for", TOK_FOR);
        KW("foreach", TOK_FOREACH);
        KW("do", TOK_DO);
        KW("switch", TOK_SWITCH);
        KW("case", TOK_CASE);
        KW("default", TOK_DEFAULT);
        KW("function", TOK_FUNCTION);
        KW("global", TOK_GLOBAL);
        KW("return", TOK_RETURN);
        KW("break", TOK_BREAK);
        KW("continue", TOK_CONTINUE);
        KW("unset", TOK_UNSET);
        KW("as", TOK_AS);
        KW("null", TOK_NULL);
        KW("undefined", TOK_UNDEFINED);
        KW("void", TOK_VOID);
        KW("true", TOK_TRUE);
        KW("false", TOK_FALSE);
    }

    Token tok = make_token(l, is_var ? TOK_VAR : TOK_IDENT);
    tok.string_val = strndup(start, len);
    return tok;
}

void lexer_init(Lexer *lx, const char *source, const char *filename) {
    lx->src  = source;
    lx->cur  = source;
    lx->filename = filename;
    lx->line = 1;
    lx->col  = 1;
    lx->start_line = 1;
    lx->start_col = 1;
}

Token lexer_next(Lexer *l) {
    skip_spaces(l);
    l->start_line = l->line;
    l->start_col = l->col;

    char c = advance(l);
    if (!c) return make_token(l, TOK_EOF);

    if (c == '"') return dstring(l);
    if (c == '\'') return sstring(l);

    if (isdigit(c) || (c == '.' && isdigit(peek(l)))) {
        l->cur--;
        return number(l);
    }

    if (c == '$') {
        if (peek(l) == '$') return make_token(l, TOK_DOLLAR);
        return identifier(l, 1);
    }
    if (isalpha(c) || c == '_') {
        l->cur--;
        return identifier(l, 0);
    }

    switch (c) {
        case '=':
            if (match(l,'>')) return make_token(l, TOK_ARROW);
            return match(l,'=') ? (match(l,'=') ? make_token(l,TOK_SEQ) : make_token(l,TOK_EQ)) : make_token(l,TOK_ASSIGN);
        case '!': return match(l,'=') ? (match(l,'=') ? make_token(l,TOK_SNEQ) : make_token(l,TOK_NEQ)) : make_token(l,TOK_NOT);
        case '<': return match(l,'<') ? make_token(l,TOK_SHL) : (match(l,'=') ? make_token(l,TOK_LTE) : make_token(l,TOK_LT));
        case '>': return match(l,'>') ? make_token(l,TOK_SHR) : (match(l,'=') ? make_token(l,TOK_GTE) : make_token(l,TOK_GT));
        case '&': return match(l,'&') ? make_token(l,TOK_AND) : make_token(l,TOK_BIT_AND);
        case '|': return match(l,'|') ? make_token(l,TOK_OR)  : make_token(l,TOK_BIT_OR);
        case '^': return make_token(l,TOK_BIT_XOR);
        case '~': return make_token(l,TOK_BIT_NOT);
        case '*': return match(l,'*') ? make_token(l,TOK_POW) :
                        (match(l,'=') ? make_token(l,TOK_MUL_EQ) : make_token(l,TOK_MUL));
        case '%': return make_token(l,TOK_MOD);
        case '+': return match(l,'+') ? make_token(l,TOK_PLUS_PLUS) :
                        (match(l,'=') ? make_token(l,TOK_PLUS_EQ) : make_token(l,TOK_PLUS));
        case '-': return match(l,'-') ? make_token(l,TOK_MINUS_MINUS) :
                        (match(l,'=') ? make_token(l,TOK_MINUS_EQ) : make_token(l,TOK_MINUS));
        case '/': return match(l,'=') ? make_token(l,TOK_DIV_EQ) : make_token(l,TOK_DIV);
        case '.': return match(l,'=') ? make_token(l,TOK_DOT_EQ) : make_token(l,TOK_DOT);
        case '(': return make_token(l,TOK_LPAREN);
        case ')': return make_token(l,TOK_RPAREN);
        case '{': return make_token(l,TOK_LBRACE);
        case '}': return make_token(l,TOK_RBRACE);
        case '[': return make_token(l,TOK_LBRACKET);
        case ']': return make_token(l,TOK_RBRACKET);
        case ',': return make_token(l,TOK_COMMA);
        case ';': return make_token(l,TOK_SEMI);
        case '?': return make_token(l,TOK_QUESTION);
        case ':': return make_token(l,TOK_COLON);
    }

    return error_token(l);
}
