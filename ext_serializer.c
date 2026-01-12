/**
 * @file ext_serializer.c
 * @brief PHP-compatible serialize/unserialize extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} StrBuf;

static int buf_reserve(StrBuf *b, size_t need) {
    if (b->cap >= need) return 1;
    size_t cap = b->cap ? b->cap : 64;
    while (cap < need) cap *= 2;
    char *nb = (char *)realloc(b->buf, cap);
    if (!nb) return 0;
    b->buf = nb;
    b->cap = cap;
    return 1;
}

static int buf_append_char(StrBuf *b, char c) {
    if (!buf_reserve(b, b->len + 2)) return 0;
    b->buf[b->len++] = c;
    b->buf[b->len] = '\0';
    return 1;
}

static int buf_append_str(StrBuf *b, const char *s) {
    size_t n = s ? strlen(s) : 0;
    if (!buf_reserve(b, b->len + n + 1)) return 0;
    if (n) memcpy(b->buf + b->len, s, n);
    b->len += n;
    b->buf[b->len] = '\0';
    return 1;
}

static int serialize_value(StrBuf *b, Value v);

static int serialize_string(StrBuf *b, const char *s) {
    size_t len = s ? strlen(s) : 0;
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "s:%zu:\"", len);
    if (!buf_append_str(b, tmp)) return 0;
    if (len && !buf_reserve(b, b->len + len + 2)) return 0;
    if (len) {
        memcpy(b->buf + b->len, s, len);
        b->len += len;
        b->buf[b->len] = '\0';
    }
    return buf_append_str(b, "\";");
}

static int serialize_array(StrBuf *b, Array *a) {
    size_t count = a ? a->size : 0;
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "a:%zu:{", count);
    if (!buf_append_str(b, tmp)) return 0;
    if (!a) return buf_append_char(b, '}');
    for (size_t i = 0; i < a->size; i++) {
        ArrayEntry *e = &a->entries[i];
        if (e->key.type == KEY_STRING) {
            if (!serialize_string(b, e->key.s ? e->key.s : "")) return 0;
        } else {
            char kbuf[64];
            snprintf(kbuf, sizeof(kbuf), "i:%" LX_INT_FMT ";", e->key.i);
            if (!buf_append_str(b, kbuf)) return 0;
        }
        if (!serialize_value(b, e->value)) return 0;
    }
    return buf_append_char(b, '}');
}

static int serialize_value(StrBuf *b, Value v) {
    switch (v.type) {
        case VAL_UNDEFINED:
        case VAL_VOID:
        case VAL_NULL:
            return buf_append_str(b, "N;");
        case VAL_BOOL:
            return buf_append_str(b, v.b ? "b:1;" : "b:0;");
        case VAL_INT: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "i:%" LX_INT_FMT ";", v.i);
            return buf_append_str(b, tmp);
        }
        case VAL_FLOAT: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "d:%.17g;", v.f);
            return buf_append_str(b, tmp);
        }
        case VAL_STRING:
            return serialize_string(b, v.s ? v.s : "");
        case VAL_ARRAY:
            return serialize_array(b, v.a);
        default:
            return buf_append_str(b, "N;");
    }
}

typedef struct {
    const char *cur;
} SerParser;

static void skip_ws(SerParser *p) {
    while (*p->cur && isspace((unsigned char)*p->cur)) p->cur++;
}

static int expect_char(SerParser *p, char c) {
    if (*p->cur != c) return 0;
    p->cur++;
    return 1;
}

static int parse_int(SerParser *p, lx_int_t *out) {
    const char *start = p->cur;
    if (*p->cur == '-' || *p->cur == '+') p->cur++;
    if (!isdigit((unsigned char)*p->cur)) return 0;
    while (isdigit((unsigned char)*p->cur)) p->cur++;
    char tmp[32];
    size_t n = (size_t)(p->cur - start);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, start, n);
    tmp[n] = '\0';
    *out = (lx_int_t)strtoll(tmp, NULL, 10);
    return 1;
}

static int parse_number(SerParser *p, double *out) {
    const char *start = p->cur;
    if (*p->cur == '-' || *p->cur == '+') p->cur++;
    if (!isdigit((unsigned char)*p->cur)) return 0;
    while (isdigit((unsigned char)*p->cur)) p->cur++;
    if (*p->cur == '.') {
        p->cur++;
        while (isdigit((unsigned char)*p->cur)) p->cur++;
    }
    if (*p->cur == 'e' || *p->cur == 'E') {
        p->cur++;
        if (*p->cur == '-' || *p->cur == '+') p->cur++;
        while (isdigit((unsigned char)*p->cur)) p->cur++;
    }
    char tmp[64];
    size_t n = (size_t)(p->cur - start);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, start, n);
    tmp[n] = '\0';
    *out = strtod(tmp, NULL);
    return 1;
}

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static Value parse_value(SerParser *p, int *ok);

static Value parse_string(SerParser *p, int *ok) {
    if (!expect_char(p, 's') || !expect_char(p, ':')) { *ok = 0; return value_undefined(); }
    lx_int_t len = 0;
    if (!parse_int(p, &len)) { *ok = 0; return value_undefined(); }
    if (!expect_char(p, ':') || !expect_char(p, '"')) { *ok = 0; return value_undefined(); }
    if (len < 0) { *ok = 0; return value_undefined(); }
    size_t n = (size_t)len;
    const char *start = p->cur;
    size_t avail = strlen(p->cur);
    if (avail < n + 2) { *ok = 0; return value_undefined(); }
    p->cur += n;
    if (!expect_char(p, '"') || !expect_char(p, ';')) { *ok = 0; return value_undefined(); }
    char *s = dup_range(start, n);
    if (!s) { *ok = 0; return value_undefined(); }
    Value out = value_string(s);
    free(s);
    return out;
}

static Value parse_array(SerParser *p, int *ok) {
    if (!expect_char(p, 'a') || !expect_char(p, ':')) { *ok = 0; return value_undefined(); }
    lx_int_t count = 0;
    if (!parse_int(p, &count)) { *ok = 0; return value_undefined(); }
    if (!expect_char(p, ':') || !expect_char(p, '{')) { *ok = 0; return value_undefined(); }
    Value out = value_array();
    for (lx_int_t i = 0; i < count; i++) {
        Value key = parse_value(p, ok);
        if (!*ok) { value_free(out); return value_undefined(); }
        Value val = parse_value(p, ok);
        if (!*ok) { value_free(key); value_free(out); return value_undefined(); }
        if (key.type == VAL_STRING) {
            array_set(out.a, key_string(key.s ? key.s : ""), val);
        } else if (key.type == VAL_INT) {
            array_set(out.a, key_int(key.i), val);
        } else {
            value_free(key);
            value_free(val);
            value_free(out);
            *ok = 0;
            return value_undefined();
        }
        value_free(key);
    }
    if (!expect_char(p, '}')) { value_free(out); *ok = 0; return value_undefined(); }
    return out;
}

static Value parse_value(SerParser *p, int *ok) {
    skip_ws(p);
    if (*p->cur == 'N') {
        p->cur++;
        if (!expect_char(p, ';')) { *ok = 0; return value_undefined(); }
        return value_null();
    }
    if (*p->cur == 'b') {
        p->cur++;
        if (!expect_char(p, ':')) { *ok = 0; return value_undefined(); }
        if (*p->cur != '0' && *p->cur != '1') { *ok = 0; return value_undefined(); }
        int v = (*p->cur == '1');
        p->cur++;
        if (!expect_char(p, ';')) { *ok = 0; return value_undefined(); }
        return value_bool(v);
    }
    if (*p->cur == 'i') {
        p->cur++;
        if (!expect_char(p, ':')) { *ok = 0; return value_undefined(); }
        lx_int_t v = 0;
        if (!parse_int(p, &v)) { *ok = 0; return value_undefined(); }
        if (!expect_char(p, ';')) { *ok = 0; return value_undefined(); }
        return value_int(v);
    }
    if (*p->cur == 'd') {
        p->cur++;
        if (!expect_char(p, ':')) { *ok = 0; return value_undefined(); }
        double v = 0.0;
        if (!parse_number(p, &v)) { *ok = 0; return value_undefined(); }
        if (!expect_char(p, ';')) { *ok = 0; return value_undefined(); }
        return value_float(v);
    }
    if (*p->cur == 's') return parse_string(p, ok);
    if (*p->cur == 'a') return parse_array(p, ok);
    *ok = 0;
    return value_undefined();
}

static Value n_serialize(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_string("N;");
    StrBuf b = {0};
    if (!serialize_value(&b, argv[0])) {
        free(b.buf);
        return value_string("N;");
    }
    Value out = value_string(b.buf ? b.buf : "");
    free(b.buf);
    return out;
}

static Value n_unserialize(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    SerParser p = { argv[0].s ? argv[0].s : "" };
    int ok = 1;
    Value out = parse_value(&p, &ok);
    skip_ws(&p);
    if (!ok || *p.cur != '\0') {
        value_free(out);
        return value_undefined();
    }
    return out;
}

static void serializer_module_init(Env *global){
    lx_register_function("serialize", n_serialize);
    lx_register_function("unserialize", n_unserialize);
    (void)global;
}

void register_serializer_module(void) {
    lx_register_extension("serializer");
    lx_register_module(serializer_module_init);
}
