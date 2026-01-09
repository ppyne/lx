/**
 * @file parser.c
 * @brief Parser implementation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "lx_error.h"

/* ---------- helpers ---------- */

#define RETURN_IF_ERROR(p) do { if (lx_has_error()) return NULL; } while (0)

static const char *token_name(TokenType t) {
    switch (t) {
        case TOK_EOF: return "end of file";
        case TOK_ERROR: return "invalid token";
        case TOK_INT: return "int";
        case TOK_FLOAT: return "float";
        case TOK_STRING: return "string";
        case TOK_DSTRING: return "string";
        case TOK_IDENT: return "identifier";
        case TOK_VAR: return "variable";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_WHILE: return "while";
        case TOK_FOR: return "for";
        case TOK_FOREACH: return "foreach";
        case TOK_DO: return "do";
        case TOK_SWITCH: return "switch";
        case TOK_CASE: return "case";
        case TOK_DEFAULT: return "default";
        case TOK_FUNCTION: return "function";
        case TOK_RETURN: return "return";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
        case TOK_UNSET: return "unset";
        case TOK_AS: return "as";
        case TOK_NULL: return "null";
        case TOK_UNDEFINED: return "undefined";
        case TOK_VOID: return "void";
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_ASSIGN: return "=";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_MUL: return "*";
        case TOK_DIV: return "/";
        case TOK_MOD: return "%";
        case TOK_POW: return "**";
        case TOK_DOT: return ".";
        case TOK_PLUS_PLUS: return "++";
        case TOK_MINUS_MINUS: return "--";
        case TOK_PLUS_EQ: return "+=";
        case TOK_MINUS_EQ: return "-=";
        case TOK_MUL_EQ: return "*=";
        case TOK_DIV_EQ: return "/=";
        case TOK_DOT_EQ: return ".=";
        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_SEQ: return "===";
        case TOK_SNEQ: return "!==";
        case TOK_LT: return "<";
        case TOK_GT: return ">";
        case TOK_LTE: return "<=";
        case TOK_GTE: return ">=";
        case TOK_AND: return "&&";
        case TOK_OR: return "||";
        case TOK_NOT: return "!";
        case TOK_DOLLAR: return "$";
        case TOK_BIT_AND: return "&";
        case TOK_BIT_OR: return "|";
        case TOK_BIT_XOR: return "^";
        case TOK_BIT_NOT: return "~";
        case TOK_SHL: return "<<";
        case TOK_SHR: return ">>";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_COMMA: return ",";
        case TOK_SEMI: return ";";
        case TOK_QUESTION: return "?";
        case TOK_COLON: return ":";
        case TOK_ARROW: return "=>";
        default: return "token";
    }
}

static void token_desc(Token tok, char *buf, size_t n) {
    if (n == 0) return;
    switch (tok.type) {
        case TOK_INT:
            snprintf(buf, n, "int %d", tok.int_val);
            break;
        case TOK_FLOAT:
            snprintf(buf, n, "float %g", tok.float_val);
            break;
        case TOK_STRING:
        case TOK_DSTRING: {
            const char *s = tok.string_val ? tok.string_val : "";
            char tmp[32];
            size_t j = 0;
            for (size_t i = 0; s[i] && j < sizeof(tmp) - 1; i++) {
                unsigned char c = (unsigned char)s[i];
                tmp[j++] = isprint(c) ? (char)c : '?';
            }
            tmp[j] = '\0';
            snprintf(buf, n, "string \"%s\"", tmp);
            break;
        }
        case TOK_IDENT:
            snprintf(buf, n, "identifier '%s'", tok.string_val ? tok.string_val : "");
            break;
        case TOK_VAR:
            snprintf(buf, n, "variable '$%s'", tok.string_val ? tok.string_val : "");
            break;
        default:
            snprintf(buf, n, "%s", token_name(tok.type));
            break;
    }
}

static void parse_error(Parser *p, const char *msg) {
    char got[64];
    token_desc(p->current, got, sizeof(got));
    lx_set_error(LX_ERR_PARSE, p->current.line, p->current.col,
                 "%s (got %s)", msg, got);
}

static void advance(Parser *p) {
    p->previous = p->current;
    p->current = lexer_next(&p->lexer);
}

static int check(Parser *p, TokenType t) {
    return p->current.type == t;
}

static int match(Parser *p, TokenType t) {
    if (!check(p, t)) return 0;
    advance(p);
    return 1;
}

static void expect(Parser *p, TokenType t, const char *msg) {
    if (!match(p, t)) {
        char expected[64];
        snprintf(expected, sizeof(expected), "%s (expected %s)", msg, token_name(t));
        parse_error(p, expected);
    }
}

static AstNode *node(Parser *p, AstType t) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    n->type = t;
    Token *src = (p->previous.type != TOK_ERROR) ? &p->previous : &p->current;
    n->line = src->line;
    n->col = src->col;
    return n;
}

/* ---------- precedence ---------- */

typedef enum {
    PREC_NONE = 0,
    PREC_ASSIGN,      /* = */
    PREC_OR,          /* || */
    PREC_AND,         /* && */
    PREC_EQUAL,       /* == != === !== */
    PREC_COMPARE,     /* < <= > >= */
    PREC_BIT_OR,      /* | */
    PREC_BIT_XOR,     /* ^ */
    PREC_BIT_AND,     /* & */
    PREC_SHIFT,       /* << >> */
    PREC_CONCAT,      /* . */
    PREC_TERM,        /* + - */
    PREC_FACTOR,      /* * / */
    PREC_POWER,       /* ** (right associative) */
    PREC_UNARY,       /* ! ~ - */
    PREC_CALL,        /* () [] */
    PREC_PRIMARY
} Precedence;

static Precedence precedence(TokenType t) {
    switch (t) {
        case TOK_ASSIGN: return PREC_NONE;
        case TOK_OR: return PREC_OR;
        case TOK_AND: return PREC_AND;

        case TOK_EQ: case TOK_NEQ:
        case TOK_SEQ: case TOK_SNEQ:
            return PREC_EQUAL;

        case TOK_LT: case TOK_LTE:
        case TOK_GT: case TOK_GTE:
            return PREC_COMPARE;

        case TOK_BIT_OR: return PREC_BIT_OR;
        case TOK_BIT_XOR: return PREC_BIT_XOR;
        case TOK_BIT_AND: return PREC_BIT_AND;

        case TOK_SHL: case TOK_SHR:
            return PREC_SHIFT;

        case TOK_DOT:
            return PREC_CONCAT;

        case TOK_PLUS: case TOK_MINUS:
            return PREC_TERM;

        case TOK_MUL: case TOK_DIV: case TOK_MOD:
            return PREC_FACTOR;

        case TOK_POW:
            return PREC_POWER;

        default:
            return PREC_NONE;
    }
}

static Operator op_from_token(TokenType t) {
    switch (t) {
        case TOK_PLUS: return OP_ADD;
        case TOK_MINUS: return OP_SUB;
        case TOK_MUL: return OP_MUL;
        case TOK_DIV: return OP_DIV;
        case TOK_MOD: return OP_MOD;
        case TOK_POW: return OP_POW;
        case TOK_DOT: return OP_CONCAT;

        case TOK_EQ: return OP_EQ;
        case TOK_NEQ: return OP_NEQ;
        case TOK_SEQ: return OP_SEQ;
        case TOK_SNEQ: return OP_SNEQ;

        case TOK_LT: return OP_LT;
        case TOK_LTE: return OP_LTE;
        case TOK_GT: return OP_GT;
        case TOK_GTE: return OP_GTE;

        case TOK_AND: return OP_AND;
        case TOK_OR: return OP_OR;

        case TOK_BIT_AND: return OP_BIT_AND;
        case TOK_BIT_OR: return OP_BIT_OR;
        case TOK_BIT_XOR: return OP_BIT_XOR;
        case TOK_SHL: return OP_SHL;
        case TOK_SHR: return OP_SHR;

        default:
            fprintf(stderr, "unknown operator\n");
            exit(1);
    }
}

static int is_assign_op(TokenType t, Operator *op_out) {
    switch (t) {
        case TOK_PLUS_EQ:  *op_out = OP_ADD; return 1;
        case TOK_MINUS_EQ: *op_out = OP_SUB; return 1;
        case TOK_MUL_EQ:   *op_out = OP_MUL; return 1;
        case TOK_DIV_EQ:   *op_out = OP_DIV; return 1;
        case TOK_DOT_EQ:   *op_out = OP_CONCAT; return 1;
        default: return 0;
    }
}

static AstNode *make_int_literal(Parser *p, int v) {
    AstNode *n = node(p, AST_LITERAL);
    n->literal.token.type = TOK_INT;
    n->literal.token.int_val = v;
    return n;
}

/* ---------- forward ---------- */
static AstNode *parse_expression(Parser *p, Precedence prec);
static AstNode *parse_expression_with_left(Parser *p, AstNode *left, Precedence prec);
static AstNode *parse_unary(Parser *p);

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

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static AstNode *make_string_literal(Parser *p, const char *s, size_t n) {
    AstNode *nnode = node(p, AST_LITERAL);
    Token tok;
    tok.type = TOK_STRING;
    tok.line = p->previous.line;
    tok.col = p->previous.col;
    tok.string_val = (char *)malloc(n + 1);
    if (!tok.string_val) tok.string_val = strdup("");
    if (tok.string_val) {
        memcpy(tok.string_val, s, n);
        tok.string_val[n] = '\0';
    }
    nnode->literal.token = tok;
    return nnode;
}

static const char *current_filename(Parser *p) {
    return p->lexer.filename ? p->lexer.filename : "";
}

static char *dirname_copy(const char *path) {
    if (!path || !*path) return strdup(".");
    const char *last = strrchr(path, '/');
    if (!last) return strdup(".");
    if (last == path) return strdup("/");
    size_t len = (size_t)(last - path);
    char *out = (char *)malloc(len + 1);
    if (!out) return strdup(".");
    memcpy(out, path, len);
    out[len] = '\0';
    return out;
}

static AstNode *concat_nodes(Parser *p, AstNode *left, AstNode *right) {
    if (!left) return right;
    if (!right) return left;
    AstNode *b = node(p, AST_BINARY);
    b->binary.op = OP_CONCAT;
    b->binary.left = left;
    b->binary.right = right;
    return b;
}

static AstNode *parse_destruct_target(Parser *p) {
    AstNode *t = parse_unary(p);
    RETURN_IF_ERROR(p);

    if (t->type == AST_VAR || t->type == AST_VAR_DYNAMIC) return t;
    if (t->type == AST_INDEX) {
        AstNode *cur = t;
        while (cur && cur->type == AST_INDEX) cur = cur->index.target;
        if (cur && (cur->type == AST_VAR || cur->type == AST_VAR_DYNAMIC)) {
            return t;
        }
    }

    parse_error(p, "destructuring target must be variable or indexed element");
    RETURN_IF_ERROR(p);
    return NULL;
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static char *normalize_interp_expr(const char *s, size_t n) {
    Lexer lx;
    lexer_init(&lx, s, NULL);
    Token t1 = lexer_next(&lx);
    if (t1.type != TOK_IDENT) return dup_range(s, n);
    Token t2 = lexer_next(&lx);
    if (t2.type == TOK_LPAREN) return dup_range(s, n);
    char *out = (char *)malloc(n + 2);
    if (!out) return NULL;
    out[0] = '$';
    memcpy(out + 1, s, n);
    out[n + 1] = '\0';
    return out;
}

static char *unescape_interp_expr(const char *s, size_t n, size_t *out_len) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\\' && i + 1 < n) {
            out[j++] = s[i + 1];
            i++;
        } else {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    if (out_len) *out_len = j;
    return out;
}

static AstNode *parse_interp_with(const char *expr_src, const char *filename) {
    Parser sub;
    lexer_init(&sub.lexer, expr_src, filename);
    sub.current.type = TOK_ERROR;
    sub.previous.type = TOK_ERROR;
    advance(&sub);
    AstNode *expr = parse_expression(&sub, PREC_ASSIGN);
    if (lx_has_error()) return NULL;
    if (sub.current.type != TOK_EOF) return NULL;
    return expr;
}

static AstNode *parse_interp_expression(Parser *p, const char *s, size_t n) {
    size_t ulen = 0;
    char *unesc = unescape_interp_expr(s, n, &ulen);
    if (!unesc) return NULL;

    lx_error_clear();
    AstNode *expr = parse_interp_with(unesc, p->lexer.filename);
    if (expr) {
        free(unesc);
        return expr;
    }

    lx_error_clear();
    char *expr_src = normalize_interp_expr(unesc, ulen);
    free(unesc);
    if (!expr_src) return NULL;
    expr = parse_interp_with(expr_src, p->lexer.filename);
    free(expr_src);
    if (!expr) {
        parse_error(p, "invalid interpolation expression");
        return NULL;
    }
    return expr;
}

static AstNode *parse_dstring(Parser *p, const char *raw) {
    AstNode *expr = NULL;
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    for (size_t i = 0; raw[i]; i++) {
        char c = raw[i];
        if (c == '\\') {
            char n = raw[i + 1];
            if (!n) {
                if (!append_char(&buf, &cap, &len, '\\')) return NULL;
                break;
            }
            i++;
            if (n == 'n') { if (!append_char(&buf, &cap, &len, '\n')) return NULL; }
            else if (n == 't') { if (!append_char(&buf, &cap, &len, '\t')) return NULL; }
            else if (n == 'r') { if (!append_char(&buf, &cap, &len, '\r')) return NULL; }
            else if (n == '"' || n == '\\' || n == '$') {
                if (!append_char(&buf, &cap, &len, n)) return NULL;
            } else if (n == 'x') {
                char h1 = raw[i + 1];
                char h2 = raw[i + 2];
                int v1 = isxdigit((unsigned char)h1) ? (isdigit((unsigned char)h1) ? h1 - '0' : (tolower((unsigned char)h1) - 'a' + 10)) : -1;
                int v2 = isxdigit((unsigned char)h2) ? (isdigit((unsigned char)h2) ? h2 - '0' : (tolower((unsigned char)h2) - 'a' + 10)) : -1;
                if (v1 >= 0 && v2 >= 0) {
                    i += 2;
                    if (!append_char(&buf, &cap, &len, (char)((v1 << 4) | v2))) return NULL;
                } else {
                    if (!append_char(&buf, &cap, &len, 'x')) return NULL;
                }
            } else {
                if (!append_char(&buf, &cap, &len, n)) return NULL;
            }
            continue;
        }
        if (c == '$') {
            char n = raw[i + 1];
            if (n == '{') {
                size_t start = i + 2;
                size_t end = start;
                while (raw[end] && raw[end] != '}') end++;
                if (!raw[end]) {
                    if (!append_char(&buf, &cap, &len, '$')) return NULL;
                    continue;
                }
                if (len > 0) {
                    AstNode *lit = make_string_literal(p, buf, len);
                    expr = concat_nodes(p, expr, lit);
                    len = 0;
                }
                AstNode *iexpr = parse_interp_expression(p, raw + start, end - start);
                if (!iexpr) { free(buf); return NULL; }
                expr = concat_nodes(p, expr, iexpr);
                i = end;
                continue;
            }
            if (is_ident_start(n)) {
                size_t start = i + 1;
                size_t end = start + 1;
                while (raw[end] && is_ident_char(raw[end])) end++;
                if (len > 0) {
                    AstNode *lit = make_string_literal(p, buf, len);
                    expr = concat_nodes(p, expr, lit);
                    len = 0;
                }
                char *name = dup_range(raw + start, end - start);
                AstNode *var = node(p, AST_VAR);
                var->var.name = name;
                expr = concat_nodes(p, expr, var);
                i = end - 1;
                continue;
            }
        }
        if (!append_char(&buf, &cap, &len, c)) return NULL;
    }

    if (len > 0 || !expr) {
        AstNode *lit = make_string_literal(p, buf ? buf : "", len);
        expr = concat_nodes(p, expr, lit);
    }
    free(buf);
    return expr;
}

static AstNode *parse_for_assign(Parser *p, char *name) {
    RETURN_IF_ERROR(p);
    Operator op = OP_ASSIGN;
    AstNode *n = node(p, AST_ASSIGN);
    n->assign.name = name;

    if (match(p, TOK_ASSIGN)) {
        AstNode *val = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        n->assign.value = val;
        n->assign.is_compound = 0;
        n->assign.op = OP_ASSIGN;
        return n;
    }
    if (is_assign_op(p->current.type, &op)) {
        advance(p);
        AstNode *val = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        n->assign.value = val;
        n->assign.is_compound = 1;
        n->assign.op = op;
        return n;
    }
    if (match(p, TOK_PLUS_PLUS) || match(p, TOK_MINUS_MINUS)) {
        int inc = (p->previous.type == TOK_PLUS_PLUS) ? 1 : -1;
        AstNode *val = make_int_literal(p, 1);
        n->assign.value = val;
        n->assign.is_compound = 1;
        n->assign.op = (inc > 0) ? OP_ADD : OP_SUB;
        return n;
    }

    parse_error(p, "for clause must be assignment");
    RETURN_IF_ERROR(p);
    return NULL;
}

static AstNode *parse_for_clause(Parser *p) {
    RETURN_IF_ERROR(p);
    AstNode **items = NULL;
    int count = 0;

    do {
        if (!check(p, TOK_VAR)) {
            parse_error(p, "for clause must start with a variable");
            RETURN_IF_ERROR(p);
        }
        advance(p);
        char *name = strdup(p->previous.string_val);
        AstNode *one = parse_for_assign(p, name);
        RETURN_IF_ERROR(p);
        items = realloc(items, sizeof(AstNode *) * (count + 1));
        items[count++] = one;
    } while (match(p, TOK_COMMA));

    if (count == 1) return items[0];

    AstNode *blk = node(p, AST_BLOCK);
    blk->block.items = items;
    blk->block.count = count;
    return blk;
}

/* ---------- prefix ---------- */

static AstNode *parse_primary(Parser *p) {
    RETURN_IF_ERROR(p);
    if (match(p, TOK_DSTRING)) {
        return parse_dstring(p, p->previous.string_val ? p->previous.string_val : "");
    }
    /* literals */
    if (match(p, TOK_INT) || match(p, TOK_FLOAT) ||
        match(p, TOK_STRING) || match(p, TOK_TRUE) ||
        match(p, TOK_FALSE) || match(p, TOK_NULL) ||
        match(p, TOK_UNDEFINED) || match(p, TOK_VOID)) {
        AstNode *n = node(p, AST_LITERAL);
        n->literal.token = p->previous;
        return n;
    }

    /* array literal */
    if (match(p, TOK_LBRACKET)) {
        AstNode *n = node(p, AST_ARRAY_LITERAL);
        n->array.keys = NULL;
        n->array.values = NULL;
        n->array.count = 0;

        if (!check(p, TOK_RBRACKET)) {
            do {
                AstNode *first = parse_expression(p, PREC_ASSIGN);
                RETURN_IF_ERROR(p);
                AstNode *key = NULL;
                AstNode *val = first;

                if (match(p, TOK_ARROW)) {
                    key = first;
                    val = parse_expression(p, PREC_ASSIGN);
                    RETURN_IF_ERROR(p);
                }

                n->array.keys = realloc(n->array.keys, sizeof(AstNode *) * (n->array.count + 1));
                n->array.values = realloc(n->array.values, sizeof(AstNode *) * (n->array.count + 1));
                n->array.keys[n->array.count] = key;
                n->array.values[n->array.count] = val;
                n->array.count++;

                if (check(p, TOK_RBRACKET)) break;
            } while (match(p, TOK_COMMA));
        }

        expect(p, TOK_RBRACKET, "]");
        RETURN_IF_ERROR(p);
        return n;
    }

    /* variable */
    if (match(p, TOK_VAR)) {
        AstNode *n = node(p, AST_VAR);
        n->var.name = strdup(p->previous.string_val);
        return n;
    }

    /* magic constants */
    if (check(p, TOK_IDENT)) {
        const char *name = p->current.string_val;
        if (name && strcmp(name, "__LINE__") == 0) {
            advance(p);
            AstNode *n = node(p, AST_LITERAL);
            n->literal.token.type = TOK_INT;
            n->literal.token.line = p->previous.line;
            n->literal.token.col = p->previous.col;
            n->literal.token.int_val = p->previous.line;
            return n;
        }
        if (name && strcmp(name, "__FILE__") == 0) {
            advance(p);
            const char *fname = current_filename(p);
            return make_string_literal(p, fname, strlen(fname));
        }
        if (name && strcmp(name, "__DIR__") == 0) {
            advance(p);
            char *dir = dirname_copy(current_filename(p));
            AstNode *n = make_string_literal(p, dir, strlen(dir));
            free(dir);
            return n;
        }
        if (name && strcmp(name, "__FUNCTION__") == 0) {
            advance(p);
            return node(p, AST_MAGIC_FUNCTION);
        }
    }

    /* identifier → call */
    if (match(p, TOK_IDENT)) {
        char *name = strdup(p->previous.string_val);
        if (match(p, TOK_LPAREN)) {
            AstNode *n = node(p, AST_CALL);
            n->call.name = name;
            n->call.args = NULL;
            n->call.argc = 0;

            if (!check(p, TOK_RPAREN)) {
                do {
                    n->call.args = realloc(n->call.args,
                        sizeof(AstNode *) * (n->call.argc + 1));
                    n->call.args[n->call.argc++] =
                        parse_expression(p, PREC_ASSIGN);
                    RETURN_IF_ERROR(p);
                } while (match(p, TOK_COMMA));
            }
            expect(p, TOK_RPAREN, ")");
            RETURN_IF_ERROR(p);
            return n;
        }
        /* bare identifier not allowed */
        parse_error(p, "unexpected identifier");
        RETURN_IF_ERROR(p);
    }

    /* grouping */
    if (match(p, TOK_LPAREN)) {
        AstNode *e = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        return e;
    }

    parse_error(p, "unexpected token");
    RETURN_IF_ERROR(p);
    return NULL;
}

/* ---------- postfix (indexing) ---------- */

static AstNode *parse_postfix(Parser *p)
{
    RETURN_IF_ERROR(p);
    AstNode *n = parse_primary(p);
    RETURN_IF_ERROR(p);

    for (;;) {
        if (match(p, TOK_LBRACKET)) {          /* '[' */
            AstNode *idx = parse_expression(p, PREC_ASSIGN);
            expect(p, TOK_RBRACKET, "]");      /* ']' */
            RETURN_IF_ERROR(p);

            AstNode *ix = node(p, AST_INDEX);
            ix->index.target = n;
            ix->index.index  = idx;
            n = ix;
            continue;
        }
        break;
    }
    if (match(p, TOK_PLUS_PLUS)) {
        if (n->type != AST_VAR && n->type != AST_INDEX && n->type != AST_VAR_DYNAMIC) {
            parse_error(p, "++ expects variable or indexed element");
            RETURN_IF_ERROR(p);
        }
        AstNode *ix = node(p, AST_POST_INC);
        ix->incdec.target = n;
        return ix;
    }
    if (match(p, TOK_MINUS_MINUS)) {
        if (n->type != AST_VAR && n->type != AST_INDEX && n->type != AST_VAR_DYNAMIC) {
            parse_error(p, "-- expects variable or indexed element");
            RETURN_IF_ERROR(p);
        }
        AstNode *ix = node(p, AST_POST_DEC);
        ix->incdec.target = n;
        return ix;
    }
    return n;
}

/* ---------- unary ---------- */

static AstNode *parse_unary(Parser *p) {
    RETURN_IF_ERROR(p);
    if (match(p, TOK_PLUS_PLUS)) {
        AstNode *t = parse_unary(p);
        RETURN_IF_ERROR(p);
        if (t->type != AST_VAR && t->type != AST_INDEX && t->type != AST_VAR_DYNAMIC) {
            parse_error(p, "++ expects variable or indexed element");
            RETURN_IF_ERROR(p);
        }
        AstNode *n = node(p, AST_PRE_INC);
        n->incdec.target = t;
        return n;
    }
    if (match(p, TOK_MINUS_MINUS)) {
        AstNode *t = parse_unary(p);
        RETURN_IF_ERROR(p);
        if (t->type != AST_VAR && t->type != AST_INDEX && t->type != AST_VAR_DYNAMIC) {
            parse_error(p, "-- expects variable or indexed element");
            RETURN_IF_ERROR(p);
        }
        AstNode *n = node(p, AST_PRE_DEC);
        n->incdec.target = t;
        return n;
    }
    if (match(p, TOK_DOLLAR)) {
        AstNode *t = parse_unary(p);
        RETURN_IF_ERROR(p);
        AstNode *n = node(p, AST_VAR_DYNAMIC);
        n->var_dynamic.expr = t;
        return n;
    }
    if (match(p, TOK_NOT)) {
        AstNode *n = node(p, AST_UNARY);
        n->unary.op = OP_NOT;
        n->unary.expr = parse_unary(p);
        RETURN_IF_ERROR(p);
        return n;
    }
    if (match(p, TOK_BIT_NOT)) {
        AstNode *n = node(p, AST_UNARY);
        n->unary.op = OP_BIT_NOT;
        n->unary.expr = parse_unary(p);
        RETURN_IF_ERROR(p);
        return n;
    }
    if (match(p, TOK_MINUS)) {
        AstNode *n = node(p, AST_UNARY);
        n->unary.op = OP_SUB;
        n->unary.expr = parse_unary(p);
        RETURN_IF_ERROR(p);
        return n;
    }
    return parse_postfix(p);
}

/* ---------- infix / Pratt ---------- */

static AstNode *parse_expression(Parser *p, Precedence prec) {
    RETURN_IF_ERROR(p);
    AstNode *left = parse_unary(p);
    RETURN_IF_ERROR(p);

    return parse_expression_with_left(p, left, prec);
}

static AstNode *parse_expression_with_left(Parser *p, AstNode *left, Precedence prec) {
    RETURN_IF_ERROR(p);
    for (;;) {
        Precedence pcur = precedence(p->current.type);
        if (pcur < prec) break;

        TokenType op_tok = p->current.type;
        advance(p);

        /* right associativity for ** */
        Precedence next_prec =
            (op_tok == TOK_POW) ? (pcur) : (pcur + 1);

        AstNode *right = parse_expression(p, next_prec);
        RETURN_IF_ERROR(p);

    AstNode *b = node(p, AST_BINARY);
        b->binary.op = op_from_token(op_tok);
        b->binary.left = left;
        b->binary.right = right;
        left = b;
    }
    if (prec <= PREC_ASSIGN && match(p, TOK_QUESTION)) {
        AstNode *then_expr = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_COLON, ":");
        RETURN_IF_ERROR(p);
        AstNode *else_expr = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);

    AstNode *t = node(p, AST_TERNARY);
        t->ternary.cond = left;
        t->ternary.then_expr = then_expr;
        t->ternary.else_expr = else_expr;
        return t;
    }
    return left;
}

/* ---------- statements ---------- */

static AstNode *parse_statement(Parser *p);

static AstNode *parse_block(Parser *p) {
    RETURN_IF_ERROR(p);
    AstNode *b = node(p, AST_BLOCK);
    b->block.items = NULL;
    b->block.count = 0;

    expect(p, TOK_LBRACE, "{");
    RETURN_IF_ERROR(p);
    while (!check(p, TOK_RBRACE)) {
        b->block.items = realloc(b->block.items,
            sizeof(AstNode *) * (b->block.count + 1));
        b->block.items[b->block.count++] = parse_statement(p);
        RETURN_IF_ERROR(p);
    }
    expect(p, TOK_RBRACE, "}");
    RETURN_IF_ERROR(p);
    return b;
}

static AstNode *parse_case_block(Parser *p) {
    RETURN_IF_ERROR(p);
    AstNode *b = node(p, AST_BLOCK);
    b->block.items = NULL;
    b->block.count = 0;
    while (!check(p, TOK_CASE) && !check(p, TOK_DEFAULT) &&
           !check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        b->block.items = realloc(b->block.items,
            sizeof(AstNode *) * (b->block.count + 1));
        b->block.items[b->block.count++] = parse_statement(p);
        RETURN_IF_ERROR(p);
    }
    return b;
}

static AstNode *parse_statement_or_block(Parser *p) {
    RETURN_IF_ERROR(p);
    if (check(p, TOK_LBRACE)) {
        return parse_block(p);
    }
    return parse_statement(p);
}

static AstNode *parse_statement(Parser *p) {
    RETURN_IF_ERROR(p);
    /* function */
    if (match(p, TOK_FUNCTION)) {
        if (!check(p, TOK_IDENT)) {
            parse_error(p, "function name expected");
            RETURN_IF_ERROR(p);
        }
        advance(p);
        char *name = strdup(p->previous.string_val);

        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        char **params = NULL;
        AstNode **param_defaults = NULL;
        int param_count = 0;
        int saw_default = 0;
        if (!check(p, TOK_RPAREN)) {
            do {
                if (!check(p, TOK_VAR) && !check(p, TOK_IDENT)) {
                    parse_error(p, "function param name expected");
                    RETURN_IF_ERROR(p);
                }
                advance(p);
                params = realloc(params, sizeof(char *) * (param_count + 1));
                params[param_count++] = strdup(p->previous.string_val);
                param_defaults = realloc(param_defaults, sizeof(AstNode *) * param_count);
                param_defaults[param_count - 1] = NULL;

                if (match(p, TOK_ASSIGN)) {
                    AstNode *def = parse_expression(p, PREC_ASSIGN);
                    RETURN_IF_ERROR(p);
                    param_defaults[param_count - 1] = def;
                    saw_default = 1;
                } else if (saw_default) {
                    parse_error(p, "non-default parameter after default");
                    RETURN_IF_ERROR(p);
                }
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);

        AstNode *body = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_FUNCTION);
        n->func.name = name;
        n->func.params = params;
        n->func.param_defaults = param_defaults;
        n->func.param_count = param_count;
        n->func.body = body;
        return n;
    }

    /* return */
    if (match(p, TOK_RETURN)) {
        AstNode *n = node(p, AST_RETURN);
        if (!check(p, TOK_SEMI)) {
            n->ret.value = parse_expression(p, PREC_ASSIGN);
            RETURN_IF_ERROR(p);
        } else {
            n->ret.value = NULL;
        }
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);
        return n;
    }

    /* break */
    if (match(p, TOK_BREAK)) {
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);
        return node(p, AST_BREAK);
    }

    /* continue */
    if (match(p, TOK_CONTINUE)) {
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);
        return node(p, AST_CONTINUE);
    }

    /* switch */
    if (match(p, TOK_SWITCH)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        AstNode *expr = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        expect(p, TOK_LBRACE, "{");
        RETURN_IF_ERROR(p);

        AstNode **case_exprs = NULL;
        AstNode **case_bodies = NULL;
        int case_count = 0;

        while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
            if (match(p, TOK_CASE)) {
                AstNode *ce = parse_expression(p, PREC_ASSIGN);
                RETURN_IF_ERROR(p);
                expect(p, TOK_COLON, ":");
                RETURN_IF_ERROR(p);
                AstNode *body = parse_case_block(p);
                RETURN_IF_ERROR(p);
                case_exprs = realloc(case_exprs, sizeof(AstNode *) * (case_count + 1));
                case_bodies = realloc(case_bodies, sizeof(AstNode *) * (case_count + 1));
                case_exprs[case_count] = ce;
                case_bodies[case_count] = body;
                case_count++;
                continue;
            }
            if (match(p, TOK_DEFAULT)) {
                expect(p, TOK_COLON, ":");
                RETURN_IF_ERROR(p);
                AstNode *body = parse_case_block(p);
                RETURN_IF_ERROR(p);
                case_exprs = realloc(case_exprs, sizeof(AstNode *) * (case_count + 1));
                case_bodies = realloc(case_bodies, sizeof(AstNode *) * (case_count + 1));
                case_exprs[case_count] = NULL;
                case_bodies[case_count] = body;
                case_count++;
                continue;
            }
            parse_error(p, "unexpected token in switch");
            RETURN_IF_ERROR(p);
        }
        expect(p, TOK_RBRACE, "}");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_SWITCH);
        n->switch_stmt.expr = expr;
        n->switch_stmt.case_exprs = case_exprs;
        n->switch_stmt.case_bodies = case_bodies;
        n->switch_stmt.case_count = case_count;
        return n;
    }

    /* foreach */
    if (match(p, TOK_FOREACH)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        AstNode *iterable = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_AS, "as");
        RETURN_IF_ERROR(p);

        if (!check(p, TOK_VAR)) {
            parse_error(p, "foreach expects a variable after 'as'");
            RETURN_IF_ERROR(p);
        }
        advance(p);
        char *first = strdup(p->previous.string_val);

        char *key_name = NULL;
        char *value_name = NULL;

        if (match(p, TOK_ARROW)) {
            key_name = first;
            if (!check(p, TOK_VAR)) {
                parse_error(p, "foreach expects a value variable");
                RETURN_IF_ERROR(p);
            }
            advance(p);
            value_name = strdup(p->previous.string_val);
        } else {
            value_name = first;
        }

        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        AstNode *body = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_FOREACH);
        n->foreach_stmt.iterable = iterable;
        n->foreach_stmt.key_name = key_name;
        n->foreach_stmt.value_name = value_name;
        n->foreach_stmt.body = body;
        return n;
    }

    /* for */
    if (match(p, TOK_FOR)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);

        AstNode *init = NULL;
        AstNode *cond = NULL;
        AstNode *step = NULL;

        /* init : assignment list or empty */
        if (!check(p, TOK_SEMI)) {
            init = parse_for_clause(p);
            RETURN_IF_ERROR(p);
        }
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        /* condition */
        if (!check(p, TOK_SEMI)) {
            cond = parse_expression(p, PREC_ASSIGN);
            RETURN_IF_ERROR(p);
        }
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        /* step : assignment list or empty */
        if (!check(p, TOK_RPAREN)) {
            step = parse_for_clause(p);
            RETURN_IF_ERROR(p);
        }
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);

        AstNode *body = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_FOR);
        n->for_stmt.init = init;
        n->for_stmt.cond = cond;
        n->for_stmt.step = step;
        n->for_stmt.body = body;
        return n;
    }

    /* if */
    if (match(p, TOK_IF)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        AstNode *cond = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        AstNode *then_b = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);
        AstNode *else_b = NULL;
        if (match(p, TOK_ELSE))
            else_b = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_IF);
        n->if_stmt.cond = cond;
        n->if_stmt.then_branch = then_b;
        n->if_stmt.else_branch = else_b;
        return n;
    }

    /* while */
    if (match(p, TOK_WHILE)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        AstNode *cond = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        AstNode *body = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_WHILE);
        n->while_stmt.cond = cond;
        n->while_stmt.body = body;
        return n;
    }

    /* do-while */
    if (match(p, TOK_DO)) {
        AstNode *body = parse_statement_or_block(p);
        RETURN_IF_ERROR(p);
        expect(p, TOK_WHILE, "while");
        RETURN_IF_ERROR(p);
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);
        AstNode *cond = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_DO_WHILE);
        n->do_while_stmt.body = body;
        n->do_while_stmt.cond = cond;
        return n;
    }

    /* unset */
    if (match(p, TOK_UNSET)) {
        expect(p, TOK_LPAREN, "(");
        RETURN_IF_ERROR(p);

        AstNode *target = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);

        if (target->type != AST_VAR && target->type != AST_INDEX &&
            target->type != AST_VAR_DYNAMIC) {
            parse_error(p, "unset expects variable or indexed element");
            RETURN_IF_ERROR(p);
        }

        expect(p, TOK_RPAREN, ")");
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_UNSET);
        n->unset.target = target;
        return n;
    }

    /* destructuring assignment: [$a, $b] = expr; */
    if (match(p, TOK_LBRACKET)) {
        AstNode **targets = NULL;
        int target_count = 0;
        if (!check(p, TOK_RBRACKET)) {
            do {
                AstNode *t = parse_destruct_target(p);
                RETURN_IF_ERROR(p);
                targets = realloc(targets, sizeof(AstNode *) * (target_count + 1));
                targets[target_count++] = t;
            } while (match(p, TOK_COMMA));
        }
        expect(p, TOK_RBRACKET, "]");
        RETURN_IF_ERROR(p);
        expect(p, TOK_ASSIGN, "=");
        RETURN_IF_ERROR(p);

        AstNode *v = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_DESTRUCT_ASSIGN);
        n->destruct_assign.targets = targets;
        n->destruct_assign.target_count = target_count;
        n->destruct_assign.value = v;
        return n;
    }

    /* simple variable assignment: $var = expr; */
    if (check(p, TOK_VAR)) {
        advance(p);
        char *name = strdup(p->previous.string_val);

        Operator op = OP_ASSIGN;
        if (match(p, TOK_ASSIGN)) {
            AstNode *v = parse_expression(p, PREC_ASSIGN);
            RETURN_IF_ERROR(p);
            expect(p, TOK_SEMI, ";");
            RETURN_IF_ERROR(p);

            AstNode *n = node(p, AST_ASSIGN);
            n->assign.name = name;
            n->assign.value = v;
            n->assign.is_compound = 0;
            n->assign.op = OP_ASSIGN;
            return n;
        }
        if (is_assign_op(p->current.type, &op)) {
            advance(p);
            AstNode *v = parse_expression(p, PREC_ASSIGN);
            RETURN_IF_ERROR(p);
            expect(p, TOK_SEMI, ";");
            RETURN_IF_ERROR(p);

            AstNode *n = node(p, AST_ASSIGN);
            n->assign.name = name;
            n->assign.value = v;
            n->assign.is_compound = 1;
            n->assign.op = op;
            return n;
        }
        if (match(p, TOK_PLUS_PLUS) || match(p, TOK_MINUS_MINUS)) {
            int inc = (p->previous.type == TOK_PLUS_PLUS) ? 1 : -1;
            AstNode *v = make_int_literal(p, 1);
            expect(p, TOK_SEMI, ";");
            RETURN_IF_ERROR(p);

            AstNode *n = node(p, AST_ASSIGN);
            n->assign.name = name;
            n->assign.value = v;
            n->assign.is_compound = 1;
            n->assign.op = (inc > 0) ? OP_ADD : OP_SUB;
            return n;
        }

        /* not an assignment → rollback as expression */
        /* recreate AST_VAR and continue */
        AstNode *var = node(p, AST_VAR);
        var->var.name = name;

        /* fall through to expression parsing */
        AstNode *e = var;

        /* handle possible postfix/indexing */
        while (check(p, TOK_LBRACKET)) {
            advance(p); /* '[' */
            AstNode *idx = parse_expression(p, PREC_ASSIGN);
            expect(p, TOK_RBRACKET, "]");
            RETURN_IF_ERROR(p);

            AstNode *ix = node(p, AST_INDEX);
            ix->index.target = e;
            ix->index.index = idx;
            e = ix;
        }

        /* now check for indexed assignment */
        if (match(p, TOK_ASSIGN)) {
            AstNode *v = parse_expression(p, PREC_ASSIGN);
            expect(p, TOK_SEMI, ";");

            AstNode *n = node(p, AST_INDEX_ASSIGN);
            n->index_assign.target = e;
            n->index_assign.value  = v;
            n->index_assign.is_compound = 0;
            n->index_assign.op = OP_ASSIGN;
            return n;
        }
        if (is_assign_op(p->current.type, &op)) {
            advance(p);
            AstNode *v = parse_expression(p, PREC_ASSIGN);
            expect(p, TOK_SEMI, ";");

            AstNode *n = node(p, AST_INDEX_ASSIGN);
            n->index_assign.target = e;
            n->index_assign.value  = v;
            n->index_assign.is_compound = 1;
            n->index_assign.op = op;
            return n;
        }
        if (match(p, TOK_PLUS_PLUS) || match(p, TOK_MINUS_MINUS)) {
            int inc = (p->previous.type == TOK_PLUS_PLUS) ? 1 : -1;
            AstNode *v = make_int_literal(p, 1);
            expect(p, TOK_SEMI, ";");

            AstNode *n = node(p, AST_INDEX_ASSIGN);
            n->index_assign.target = e;
            n->index_assign.value  = v;
            n->index_assign.is_compound = 1;
            n->index_assign.op = (inc > 0) ? OP_ADD : OP_SUB;
            return n;
        }

        e = parse_expression_with_left(p, e, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_EXPR_STMT);
        n->expr_stmt.expr = e;
        return n;
    }

    /* expression or indexed assignment */
    AstNode *e = parse_expression(p, PREC_ASSIGN);
    RETURN_IF_ERROR(p);

    Operator op = OP_ASSIGN;
    if (match(p, TOK_ASSIGN)) {
        AstNode *v = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        if (e->type == AST_VAR_DYNAMIC) {
            AstNode *n = node(p, AST_ASSIGN_DYNAMIC);
            n->assign_dynamic.name_expr = e->var_dynamic.expr;
            n->assign_dynamic.value = v;
            n->assign_dynamic.is_compound = 0;
            n->assign_dynamic.op = OP_ASSIGN;
            return n;
        }

        if (e->type != AST_INDEX) {
            parse_error(p, "left side of assignment is not assignable");
            RETURN_IF_ERROR(p);
        }

        AstNode *n = node(p, AST_INDEX_ASSIGN);
        n->index_assign.target = e;
        n->index_assign.value  = v;
        n->index_assign.is_compound = 0;
        n->index_assign.op = OP_ASSIGN;
        return n;
    }
    if (is_assign_op(p->current.type, &op)) {
        advance(p);
        AstNode *v = parse_expression(p, PREC_ASSIGN);
        RETURN_IF_ERROR(p);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        if (e->type == AST_VAR_DYNAMIC) {
            AstNode *n = node(p, AST_ASSIGN_DYNAMIC);
            n->assign_dynamic.name_expr = e->var_dynamic.expr;
            n->assign_dynamic.value = v;
            n->assign_dynamic.is_compound = 1;
            n->assign_dynamic.op = op;
            return n;
        }

        if (e->type != AST_INDEX) {
            parse_error(p, "left side of assignment is not assignable");
            RETURN_IF_ERROR(p);
        }

        AstNode *n = node(p, AST_INDEX_ASSIGN);
        n->index_assign.target = e;
        n->index_assign.value  = v;
        n->index_assign.is_compound = 1;
        n->index_assign.op = op;
        return n;
    }
    if (match(p, TOK_PLUS_PLUS) || match(p, TOK_MINUS_MINUS)) {
        int inc = (p->previous.type == TOK_PLUS_PLUS) ? 1 : -1;
        if (e->type != AST_INDEX) {
            parse_error(p, "left side of assignment is not assignable");
            RETURN_IF_ERROR(p);
        }
        AstNode *v = make_int_literal(p, 1);
        expect(p, TOK_SEMI, ";");
        RETURN_IF_ERROR(p);

        AstNode *n = node(p, AST_INDEX_ASSIGN);
        n->index_assign.target = e;
        n->index_assign.value  = v;
        n->index_assign.is_compound = 1;
        n->index_assign.op = (inc > 0) ? OP_ADD : OP_SUB;
        return n;
    }

    expect(p, TOK_SEMI, ";");
    RETURN_IF_ERROR(p);

    AstNode *n = node(p, AST_EXPR_STMT);
    n->expr_stmt.expr = e;
    return n;
}

/* ---------- program ---------- */

AstNode *parse_program(Parser *p) {
    advance(p);

    if (lx_has_error()) return NULL;
    AstNode *prog = node(p, AST_PROGRAM);
    prog->block.items = NULL;
    prog->block.count = 0;

    while (!check(p, TOK_EOF)) {
        prog->block.items = realloc(prog->block.items,
            sizeof(AstNode *) * (prog->block.count + 1));
        prog->block.items[prog->block.count++] = parse_statement(p);
        if (lx_has_error()) return NULL;
    }
    return prog;
}
