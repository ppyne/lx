/**
 * @file ext_json.c
 * @brief JSON extension module.
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

static int json_escape_str(StrBuf *b, const char *s) {
    if (!buf_append_char(b, '"')) return 0;
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; p++) {
        unsigned char c = *p;
        switch (c) {
            case '"':  if (!buf_append_str(b, "\\\"")) return 0; break;
            case '\\': if (!buf_append_str(b, "\\\\")) return 0; break;
            case '\b': if (!buf_append_str(b, "\\b")) return 0; break;
            case '\f': if (!buf_append_str(b, "\\f")) return 0; break;
            case '\n': if (!buf_append_str(b, "\\n")) return 0; break;
            case '\r': if (!buf_append_str(b, "\\r")) return 0; break;
            case '\t': if (!buf_append_str(b, "\\t")) return 0; break;
            default:
                if (c < 0x20) {
                    char tmp[7];
                    snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                    if (!buf_append_str(b, tmp)) return 0;
                } else {
                    if (!buf_append_char(b, (char)c)) return 0;
                }
        }
    }
    return buf_append_char(b, '"');
}

static int json_has_string_keys(Array *a) {
    if (!a) return 0;
    for (int i = 0; i < a->size; i++) {
        if (a->entries[i].key.type == KEY_STRING) return 1;
    }
    return 0;
}

static int json_encode_value(StrBuf *b, Value v);

static int json_encode_array(StrBuf *b, Array *a) {
    if (!a) return buf_append_str(b, "[]");
    int as_object = json_has_string_keys(a);
    if (!buf_append_char(b, as_object ? '{' : '[')) return 0;
    for (int i = 0; i < a->size; i++) {
        if (i > 0 && !buf_append_char(b, ',')) return 0;
        if (as_object) {
            if (a->entries[i].key.type == KEY_STRING) {
                if (!json_escape_str(b, a->entries[i].key.s ? a->entries[i].key.s : "")) return 0;
            } else {
                char tmp[32];
                snprintf(tmp, sizeof(tmp), "%d", a->entries[i].key.i);
                if (!json_escape_str(b, tmp)) return 0;
            }
            if (!buf_append_char(b, ':')) return 0;
        }
        if (!json_encode_value(b, a->entries[i].value)) return 0;
    }
    return buf_append_char(b, as_object ? '}' : ']');
}

static int json_encode_value(StrBuf *b, Value v) {
    switch (v.type) {
        case VAL_UNDEFINED:
        case VAL_VOID:
        case VAL_NULL:
            return buf_append_str(b, "null");
        case VAL_BOOL:
            return buf_append_str(b, v.b ? "true" : "false");
        case VAL_INT: {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", v.i);
            return buf_append_str(b, tmp);
        }
        case VAL_FLOAT: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%g", v.f);
            return buf_append_str(b, tmp);
        }
        case VAL_STRING:
            return json_escape_str(b, v.s ? v.s : "");
        case VAL_ARRAY:
            return json_encode_array(b, v.a);
        default:
            return buf_append_str(b, "null");
    }
}

typedef struct {
    const char *cur;
} JsonParser;

static void json_skip_ws(JsonParser *p) {
    while (*p->cur && isspace((unsigned char)*p->cur)) p->cur++;
}

static int json_match(JsonParser *p, const char *s) {
    size_t n = strlen(s);
    if (strncmp(p->cur, s, n) == 0) {
        p->cur += n;
        return 1;
    }
    return 0;
}

static Value json_parse_value(JsonParser *p, int *ok);

static Value json_parse_string(JsonParser *p, int *ok) {
    if (*p->cur != '"') { *ok = 0; return value_undefined(); }
    p->cur++;
    StrBuf b = {0};
    while (*p->cur && *p->cur != '"') {
        char c = *p->cur++;
        if (c == '\\') {
            char e = *p->cur++;
            switch (e) {
                case '"':  if (!buf_append_char(&b, '"')) { *ok = 0; return value_undefined(); } break;
                case '\\': if (!buf_append_char(&b, '\\')) { *ok = 0; return value_undefined(); } break;
                case '/':  if (!buf_append_char(&b, '/')) { *ok = 0; return value_undefined(); } break;
                case 'b':  if (!buf_append_char(&b, '\b')) { *ok = 0; return value_undefined(); } break;
                case 'f':  if (!buf_append_char(&b, '\f')) { *ok = 0; return value_undefined(); } break;
                case 'n':  if (!buf_append_char(&b, '\n')) { *ok = 0; return value_undefined(); } break;
                case 'r':  if (!buf_append_char(&b, '\r')) { *ok = 0; return value_undefined(); } break;
                case 't':  if (!buf_append_char(&b, '\t')) { *ok = 0; return value_undefined(); } break;
                case 'u': {
                    unsigned int code = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = *p->cur++;
                        if (!isxdigit((unsigned char)h)) { *ok = 0; return value_undefined(); }
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= (unsigned int)(h - '0');
                        else if (h >= 'a' && h <= 'f') code |= (unsigned int)(h - 'a' + 10);
                        else code |= (unsigned int)(h - 'A' + 10);
                    }
                    if (code <= 0x7F) {
                        if (!buf_append_char(&b, (char)code)) { *ok = 0; return value_undefined(); }
                    } else {
                        *ok = 0;
                        return value_undefined();
                    }
                    break;
                }
                default:
                    *ok = 0;
                    return value_undefined();
            }
        } else {
            if (!buf_append_char(&b, c)) { *ok = 0; return value_undefined(); }
        }
    }
    if (*p->cur != '"') { *ok = 0; return value_undefined(); }
    p->cur++;
    Value out = value_string(b.buf ? b.buf : "");
    free(b.buf);
    return out;
}

static Value json_parse_number(JsonParser *p, int *ok) {
    const char *start = p->cur;
    if (*p->cur == '-') p->cur++;
    if (!isdigit((unsigned char)*p->cur)) { *ok = 0; return value_undefined(); }
    while (isdigit((unsigned char)*p->cur)) p->cur++;
    int is_float = 0;
    if (*p->cur == '.') {
        is_float = 1;
        p->cur++;
        while (isdigit((unsigned char)*p->cur)) p->cur++;
    }
    if (*p->cur == 'e' || *p->cur == 'E') {
        is_float = 1;
        p->cur++;
        if (*p->cur == '+' || *p->cur == '-') p->cur++;
        while (isdigit((unsigned char)*p->cur)) p->cur++;
    }
    char tmp[64];
    size_t n = (size_t)(p->cur - start);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, start, n);
    tmp[n] = '\0';
    if (is_float) return value_float(strtod(tmp, NULL));
    return value_int((int)strtol(tmp, NULL, 10));
}

static Value json_parse_array(JsonParser *p, int *ok) {
    if (*p->cur != '[') { *ok = 0; return value_undefined(); }
    p->cur++;
    Value out = value_array();
    int idx = 0;
    json_skip_ws(p);
    if (*p->cur == ']') { p->cur++; return out; }
    while (*p->cur) {
        Value v = json_parse_value(p, ok);
        if (!*ok) { value_free(out); return value_undefined(); }
        array_set(out.a, key_int(idx++), v);
        json_skip_ws(p);
        if (*p->cur == ',') { p->cur++; json_skip_ws(p); continue; }
        if (*p->cur == ']') { p->cur++; return out; }
        break;
    }
    *ok = 0;
    value_free(out);
    return value_undefined();
}

static Value json_parse_object(JsonParser *p, int *ok) {
    if (*p->cur != '{') { *ok = 0; return value_undefined(); }
    p->cur++;
    Value out = value_array();
    json_skip_ws(p);
    if (*p->cur == '}') { p->cur++; return out; }
    while (*p->cur) {
        Value key = json_parse_string(p, ok);
        if (!*ok || key.type != VAL_STRING) { value_free(out); return value_undefined(); }
        json_skip_ws(p);
        if (*p->cur != ':') { value_free(key); value_free(out); *ok = 0; return value_undefined(); }
        p->cur++;
        json_skip_ws(p);
        Value v = json_parse_value(p, ok);
        if (!*ok) { value_free(key); value_free(out); return value_undefined(); }
        array_set(out.a, key_string(key.s), v);
        value_free(key);
        json_skip_ws(p);
        if (*p->cur == ',') { p->cur++; json_skip_ws(p); continue; }
        if (*p->cur == '}') { p->cur++; return out; }
        break;
    }
    *ok = 0;
    value_free(out);
    return value_undefined();
}

static Value json_parse_value(JsonParser *p, int *ok) {
    json_skip_ws(p);
    if (*p->cur == '"') return json_parse_string(p, ok);
    if (*p->cur == '{') return json_parse_object(p, ok);
    if (*p->cur == '[') return json_parse_array(p, ok);
    if (*p->cur == '-' || isdigit((unsigned char)*p->cur)) return json_parse_number(p, ok);
    if (json_match(p, "true")) return value_bool(1);
    if (json_match(p, "false")) return value_bool(0);
    if (json_match(p, "null")) return value_null();
    *ok = 0;
    return value_undefined();
}

static Value n_json_encode(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_string("null");
    StrBuf b = {0};
    if (!json_encode_value(&b, argv[0])) {
        free(b.buf);
        return value_string("null");
    }
    Value out = value_string(b.buf ? b.buf : "");
    free(b.buf);
    return out;
}

static Value n_json_decode(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    JsonParser p = { argv[0].s ? argv[0].s : "" };
    int ok = 1;
    Value out = json_parse_value(&p, &ok);
    json_skip_ws(&p);
    if (!ok || *p.cur != '\0') {
        value_free(out);
        return value_undefined();
    }
    return out;
}

static Value n_is_json(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    JsonParser p = { argv[0].s ? argv[0].s : "" };
    int ok = 1;
    Value out = json_parse_value(&p, &ok);
    value_free(out);
    json_skip_ws(&p);
    if (!ok || *p.cur != '\0') return value_bool(0);
    return value_bool(1);
}

static void json_module_init(Env *global){
    lx_register_function("json_encode", n_json_encode);
    lx_register_function("json_decode", n_json_decode);
    lx_register_function("is_json", n_is_json);
    (void)global;
}

void register_json_module(void) {
    lx_register_extension("json");
    lx_register_module(json_module_init);
}
