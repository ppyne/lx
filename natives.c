/**
 * @file natives.c
 * @brief Native function registry and implementations.
 */
#include "natives.h"
#include "env.h"
#include "gc.h"
#include "array.h"
#include "lx_ext.h"
#include "lx_error.h"
#include "parser.h"
#include "eval.h"
#include "lx_version.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>

typedef struct {
    char *name;
    NativeFn fn;
} NativeEntry;

static NativeEntry *g_fns = NULL;
static int g_count = 0;
static int g_cap = 0;
static FILE *g_output = NULL;

static void buf_append(char **buf, size_t *cap, size_t *len, const char *s, size_t n);
#if LX_ENABLE_INCLUDE
static Value run_include(Env *env, const char *path);
static int include_seen(const char *path);
static void include_mark(const char *path);
#endif

typedef struct {
    Array **stack;
    int count;
    int cap;
} DumpState;

typedef struct {
    int to_string;
    FILE *f;
    char *buf;
    size_t len;
    size_t cap;
} DumpWriter;

static void dump_indent(DumpWriter *w, int level);
static void dump_value(Value v, int indent, DumpState *st, DumpWriter *w);
static void print_r_value(Value v, int indent, DumpState *st, DumpWriter *w);

static void ensure(int need){
    if (g_cap >= need) return;
    int cap = g_cap ? g_cap : 32;
    while (cap < need) cap *= 2;
    NativeEntry *ne = (NativeEntry*)realloc(g_fns, cap*sizeof(NativeEntry));
    if (!ne) return;
    g_fns = ne; g_cap = cap;
}

void lx_set_output(FILE *f) {
    g_output = f;
}

void register_function(const char *name, NativeFn fn){
    for (int i=0;i<g_count;i++){
        if (strcmp(g_fns[i].name, name)==0){
            g_fns[i].fn = fn;
            return;
        }
    }
    ensure(g_count+1);
    g_fns[g_count].name = strdup(name);
    g_fns[g_count].fn = fn;
    g_count++;
}

NativeFn find_function(const char *name){
    for (int i=0;i<g_count;i++){
        if (strcmp(g_fns[i].name, name)==0) return g_fns[i].fn;
    }
    return NULL;
}

static Value n_print(Env *env, int argc, Value *argv){
    (void)env;
    FILE *out = g_output ? g_output : stdout;
    for (int i=0;i<argc;i++){
        Value s = value_to_string(argv[i]);
        fputs(s.s, out);
        value_free(s);
    }
    return value_void();
}

static int dump_seen(DumpState *st, Array *a) {
    for (int i = 0; i < st->count; i++) {
        if (st->stack[i] == a) return 1;
    }
    return 0;
}

static void dump_push(DumpState *st, Array *a) {
    if (st->cap <= st->count) {
        int cap = st->cap ? st->cap * 2 : 8;
        Array **ns = (Array **)realloc(st->stack, (size_t)cap * sizeof(Array *));
        if (!ns) return;
        st->stack = ns;
        st->cap = cap;
    }
    st->stack[st->count++] = a;
}

static void dump_pop(DumpState *st) {
    if (st->count > 0) st->count--;
}

static void writer_write(DumpWriter *w, const char *s, size_t n) {
    if (!w) return;
    if (!w->to_string) {
        FILE *out = w->f ? w->f : (g_output ? g_output : stdout);
        if (n > 0) fwrite(s, 1, n, out);
        return;
    }
    if (w->len + n + 1 > w->cap) {
        size_t ncap = w->cap ? w->cap * 2 : 128;
        while (ncap < w->len + n + 1) ncap *= 2;
        char *nb = (char *)realloc(w->buf, ncap);
        if (!nb) return;
        w->buf = nb;
        w->cap = ncap;
    }
    memcpy(w->buf + w->len, s, n);
    w->len += n;
    w->buf[w->len] = '\0';
}

static void writer_puts(DumpWriter *w, const char *s) {
    writer_write(w, s, strlen(s));
}

static void writer_putc(DumpWriter *w, char c) {
    if (!w->to_string) {
        FILE *out = w->f ? w->f : (g_output ? g_output : stdout);
        fputc(c, out);
        return;
    }
    if (w->len + 2 > w->cap) {
        size_t ncap = w->cap ? w->cap * 2 : 128;
        char *nb = (char *)realloc(w->buf, ncap);
        if (!nb) return;
        w->buf = nb;
        w->cap = ncap;
    }
    w->buf[w->len++] = c;
    w->buf[w->len] = '\0';
}

static void writer_printf(DumpWriter *w, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char tmp[128];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n < sizeof(tmp)) {
        writer_write(w, tmp, (size_t)n);
        return;
    }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) return;
    va_start(ap, fmt);
    vsnprintf(buf, (size_t)n + 1, fmt, ap);
    va_end(ap);
    writer_write(w, buf, (size_t)n);
    free(buf);
}

static void dump_indent(DumpWriter *w, int level) {
    for (int i = 0; i < level; i++) {
        writer_puts(w, "  ");
    }
}

static void dump_string(const char *s, size_t len, DumpWriter *w) {
    writer_puts(w, "string(");
    writer_printf(w, "%zu", len);
    writer_puts(w, ") \"");
    if (len > 0) writer_write(w, s, len);
    writer_puts(w, "\"");
}

static void dump_array(Value v, int indent, DumpState *st, DumpWriter *w) {
    Array *a = v.a;
    if (!a) {
        dump_indent(w, indent);
        writer_puts(w, "array(0) {}");
        return;
    }
    if (dump_seen(st, a)) {
        dump_indent(w, indent);
        writer_puts(w, "*RECURSION*");
        return;
    }
    dump_push(st, a);
    dump_indent(w, indent);
    writer_printf(w, "array(%d) {\n", a->size);
    for (int i = 0; i < a->size; i++) {
        ArrayEntry *e = &a->entries[i];
        dump_indent(w, indent + 1);
        if (e->key.type == KEY_STRING) {
            writer_printf(w, "[\"%s\"]=>\n", e->key.s ? e->key.s : "");
        } else {
            writer_printf(w, "[%d]=>\n", e->key.i);
        }
        dump_value(e->value, indent + 1, st, w);
        writer_putc(w, '\n');
    }
    dump_indent(w, indent);
    writer_puts(w, "}");
    dump_pop(st);
}

static void dump_value(Value v, int indent, DumpState *st, DumpWriter *w) {
    switch (v.type) {
        case VAL_UNDEFINED:
            dump_indent(w, indent);
            writer_puts(w, "undefined");
            break;
        case VAL_VOID:
            dump_indent(w, indent);
            writer_puts(w, "void");
            break;
        case VAL_NULL:
            dump_indent(w, indent);
            writer_puts(w, "NULL");
            break;
        case VAL_BOOL:
            dump_indent(w, indent);
            writer_printf(w, "bool(%s)", v.b ? "true" : "false");
            break;
        case VAL_INT:
            dump_indent(w, indent);
            writer_printf(w, "int(%d)", v.i);
            break;
        case VAL_FLOAT:
            dump_indent(w, indent);
            writer_printf(w, "float(%g)", v.f);
            break;
        case VAL_STRING: {
            const char *s = v.s ? v.s : "";
            dump_indent(w, indent);
            dump_string(s, strlen(s), w);
            break;
        }
        case VAL_ARRAY:
            dump_array(v, indent, st, w);
            break;
        default:
            dump_indent(w, indent);
            writer_puts(w, "undefined");
            break;
    }
}

static void print_r_indent(DumpWriter *w, int level) {
    for (int i = 0; i < level; i++) {
        writer_puts(w, "    ");
    }
}

static void print_r_array(Value v, int indent, DumpState *st, DumpWriter *w) {
    Array *a = v.a;
    if (!a) {
        writer_puts(w, "Array\n");
        print_r_indent(w, indent);
        writer_puts(w, "(\n");
        print_r_indent(w, indent);
        writer_puts(w, ")\n");
        return;
    }
    if (dump_seen(st, a)) {
        writer_puts(w, "*RECURSION*\n");
        return;
    }
    dump_push(st, a);
    writer_puts(w, "Array\n");
    print_r_indent(w, indent);
    writer_puts(w, "(\n");
    for (int i = 0; i < a->size; i++) {
        ArrayEntry *e = &a->entries[i];
        print_r_indent(w, indent + 1);
        if (e->key.type == KEY_STRING) {
            writer_printf(w, "[%s] => ", e->key.s ? e->key.s : "");
        } else {
            writer_printf(w, "[%d] => ", e->key.i);
        }
        if (e->value.type == VAL_ARRAY) {
            print_r_array(e->value, indent + 1, st, w);
        } else {
            print_r_value(e->value, indent + 1, st, w);
            writer_putc(w, '\n');
        }
    }
    print_r_indent(w, indent);
    writer_puts(w, ")\n");
    dump_pop(st);
}

static void print_r_value(Value v, int indent, DumpState *st, DumpWriter *w) {
    (void)indent;
    switch (v.type) {
        case VAL_UNDEFINED:
            writer_puts(w, "undefined");
            break;
        case VAL_VOID:
        case VAL_NULL:
            break;
        case VAL_BOOL:
            if (v.b) writer_puts(w, "1");
            break;
        case VAL_INT:
        case VAL_FLOAT:
        case VAL_STRING: {
            Value s = value_to_string(v);
            writer_puts(w, s.s ? s.s : "");
            value_free(s);
            break;
        }
        case VAL_ARRAY:
            print_r_array(v, indent, st, w);
            break;
        default:
            writer_puts(w, "undefined");
            break;
    }
}

static Value n_var_dump(Env *env, int argc, Value *argv){
    (void)env;
    int return_string = 0;
    if (argc >= 2 && argv[argc - 1].type == VAL_BOOL) {
        return_string = argv[argc - 1].b;
        argc--;
    }
    DumpState st = {0};
    DumpWriter w = {0};
    if (!return_string) w.f = g_output ? g_output : stdout;
    for (int i = 0; i < argc; i++) {
        dump_value(argv[i], 0, &st, &w);
        writer_putc(&w, '\n');
    }
    free(st.stack);
    if (return_string) {
        Value out = value_string(w.buf ? w.buf : "");
        free(w.buf);
        return out;
    }
    free(w.buf);
    return value_void();
}

static Value n_print_r(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1) return value_void();
    int return_string = 0;
    if (argc >= 2 && argv[1].type == VAL_BOOL) {
        return_string = argv[1].b;
    }
    DumpState st = {0};
    DumpWriter w = {0};
    if (!return_string) w.f = g_output ? g_output : stdout;
    print_r_value(argv[0], 0, &st, &w);
    free(st.stack);
    if (return_string) {
        Value out = value_string(w.buf ? w.buf : "");
        free(w.buf);
        return out;
    }
    free(w.buf);
    return value_void();
}

static Value n_strlen(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    if (argv[0].type != VAL_STRING) return value_int(0);
    return value_int(argv[0].s ? (int)strlen(argv[0].s) : 0);
}

static uint32_t crc32_table[256];
static int crc32_ready = 0;

static void crc32_init(void) {
    if (crc32_ready) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320u ^ (c >> 1);
            else c >>= 1;
        }
        crc32_table[i] = c;
    }
    crc32_ready = 1;
}

static uint32_t crc32_compute(const unsigned char *buf, size_t len) {
    crc32_init();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

static Value n_crc32(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_int(0);
    const unsigned char *s = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    uint32_t crc = crc32_compute(s, strlen((const char *)s));
    return value_int((int32_t)crc);
}

static Value n_crc32u(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("0");
    const unsigned char *s = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    uint32_t crc = crc32_compute(s, strlen((const char *)s));
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)crc);
    return value_string(buf);
}

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int base64_rev[256];
static int base64_ready = 0;

static void base64_init(void) {
    if (base64_ready) return;
    for (int i = 0; i < 256; i++) base64_rev[i] = -1;
    for (int i = 0; i < 64; i++) {
        base64_rev[(unsigned char)base64_table[i]] = i;
    }
    base64_ready = 1;
}

static Value n_base64_encode(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const unsigned char *in = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t len = strlen((const char *)in);
    size_t out_len = ((len + 2) / 3) * 4;
    char *out = (char *)malloc(out_len + 1);
    if (!out) return value_string("");

    size_t i = 0;
    size_t j = 0;
    while (i < len) {
        size_t rem = len - i;
        uint32_t a = in[i++];
        uint32_t b = (rem > 1) ? in[i++] : 0;
        uint32_t c = (rem > 2) ? in[i++] : 0;

        out[j++] = base64_table[(a >> 2) & 0x3F];
        out[j++] = base64_table[((a & 0x03) << 4) | ((b >> 4) & 0x0F)];
        out[j++] = (rem > 1) ? base64_table[((b & 0x0F) << 2) | ((c >> 6) & 0x03)] : '=';
        out[j++] = (rem > 2) ? base64_table[c & 0x3F] : '=';
    }
    out[out_len] = '\0';
    Value v = value_string(out);
    free(out);
    return v;
}

static Value n_base64_decode(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const unsigned char *in = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t len = strlen((const char *)in);
    if (len == 0) return value_string("");
    if (len % 4 != 0) return value_undefined();

    base64_init();

    size_t pad = 0;
    if (len >= 1 && in[len - 1] == '=') pad++;
    if (len >= 2 && in[len - 2] == '=') pad++;
    size_t out_len = (len / 4) * 3 - pad;
    unsigned char *out = (unsigned char *)malloc(out_len + 1);
    if (!out) return value_undefined();

    size_t i = 0;
    size_t j = 0;
    while (i < len) {
        int c0 = in[i++];
        int c1 = in[i++];
        int c2 = in[i++];
        int c3 = in[i++];

        int v0 = base64_rev[c0];
        int v1 = base64_rev[c1];
        int v2 = (c2 == '=') ? -2 : base64_rev[c2];
        int v3 = (c3 == '=') ? -2 : base64_rev[c3];

        if (v0 < 0 || v1 < 0 || v2 == -1 || v3 == -1) {
            free(out);
            return value_undefined();
        }

        if (v2 == -2 && v3 != -2) {
            free(out);
            return value_undefined();
        }

        uint32_t triple = ((uint32_t)v0 << 18) | ((uint32_t)v1 << 12);
        if (v2 >= 0) triple |= (uint32_t)v2 << 6;
        if (v3 >= 0) triple |= (uint32_t)v3;

        if (j < out_len) out[j++] = (triple >> 16) & 0xFF;
        if (v2 >= 0 && j < out_len) out[j++] = (triple >> 8) & 0xFF;
        if (v3 >= 0 && j < out_len) out[j++] = triple & 0xFF;
    }

    out[out_len] = '\0';
    Value v = value_string_n((const char *)out, out_len);
    free(out);
    return v;
}

#if LX_ENABLE_INCLUDE
static Value n_include(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    Value r = run_include(env, path);
    if (r.type == VAL_BOOL && r.b) include_mark(path);
    return r;
}

static Value n_include_once(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    if (include_seen(path)) return value_bool(1);
    Value r = run_include(env, path);
    if (r.type == VAL_BOOL && r.b) include_mark(path);
    return r;
}
#endif

static Value n_count(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    if (argv[0].type != VAL_ARRAY) return value_int(0);
    return value_int(argv[0].a ? argv[0].a->size : 0);
}

static Value n_substr(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 2 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    int len = (int)strlen(s);

    int start = (value_to_int(argv[1])).i;
    int count = (argc >= 3) ? (value_to_int(argv[2])).i : (len - start);

    if (start < 0 || start >= len) return value_string("");
    if (count <= 0) return value_string("");
    if (start + count > len) count = len - start;

    return value_string_n(s + start, (size_t)count);
}

static int is_trim_space(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void key_free_local(Key k) {
    if (k.type == KEY_STRING) free(k.s);
}

static Key key_copy_local(Key k) {
    Key out;
    out.type = k.type;
    if (k.type == KEY_STRING) {
        out.s = strdup(k.s ? k.s : "");
    } else {
        out.i = k.i;
    }
    return out;
}

int array_next_index(Array *a) {
    int next = 0;
    if (!a) return 0;
    for (int i = 0; i < a->size; i++) {
        if (a->entries[i].key.type == KEY_INT && a->entries[i].key.i >= next) {
            next = a->entries[i].key.i + 1;
        }
    }
    return next;
}

static void reindex_numeric_keys(Array *a) {
    if (!a) return;
    int next = 0;
    for (int i = 0; i < a->size; i++) {
        if (a->entries[i].key.type == KEY_INT) {
            a->entries[i].key.i = next++;
        }
    }
}

#if LX_ENABLE_INCLUDE
static char *read_file_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static char *resolve_path(const char *path) {
    if (!path || !*path) return strdup("");
    char *real = realpath(path, NULL);
    if (real) return real;
    return strdup(path);
}

static char **g_includes = NULL;
static int g_include_count = 0;
static int g_include_cap = 0;


static void include_ensure(int need) {
    if (g_include_cap >= need) return;
    int cap = g_include_cap ? g_include_cap : 8;
    while (cap < need) cap *= 2;
    char **nn = (char **)realloc(g_includes, (size_t)cap * sizeof(char *));
    if (!nn) return;
    g_includes = nn;
    g_include_cap = cap;
}

static int include_seen(const char *path) {
    for (int i = 0; i < g_include_count; i++) {
        if (strcmp(g_includes[i], path) == 0) return 1;
    }
    return 0;
}

static void include_mark(const char *path) {
    if (include_seen(path)) return;
    include_ensure(g_include_count + 1);
    g_includes[g_include_count++] = strdup(path);
}

static Value run_include(Env *env, const char *path) {
    if (!path || !*path) return value_bool(0);
    if (lx_has_error()) return value_bool(0);
    char *source = read_file_all(path);
    if (!source) {
        lx_set_error(LX_ERR_RUNTIME, 0, 0, "include: cannot read '%s'", path);
        return value_bool(0);
    }

    char *resolved = resolve_path(path);
    Parser parser;
    lexer_init(&parser.lexer, source, resolved);
    parser.current.type = TOK_ERROR;
    parser.previous.type = TOK_ERROR;

    AstNode *program = parse_program(&parser);
    if (lx_has_error() || !program) {
        free(source);
        free(resolved);
        return value_bool(0);
    }

    EvalResult r = eval_program(program, env);
    value_free(r.value);
    free(source);
    free(resolved);
    if (lx_has_error()) return value_bool(0);
    return value_bool(1);
}
#endif

static int weak_equal_native(Value a, Value b) {
    if (value_is_number(a) && value_is_number(b)) {
        return value_as_double(a) == value_as_double(b);
    }

    if (value_is_number(a) && b.type == VAL_STRING) {
        char *end;
        double v = strtod(b.s ? b.s : "", &end);
        if (*end == '\0')
            return value_as_double(a) == v;
        return 0;
    }

    if (a.type == VAL_STRING && value_is_number(b)) {
        char *end;
        double v = strtod(a.s ? a.s : "", &end);
        if (*end == '\0')
            return v == value_as_double(b);
        return 0;
    }

    if (a.type == VAL_STRING && b.type == VAL_STRING) {
        return strcmp(a.s ? a.s : "", b.s ? b.s : "") == 0;
    }

    if (a.type == VAL_BOOL && b.type == VAL_BOOL) {
        return a.b == b.b;
    }

    if (a.type == VAL_NULL && b.type == VAL_NULL) {
        return 1;
    }

    return 0;
}

static int strict_equal_native(Value a, Value b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case VAL_UNDEFINED: return 1;
        case VAL_NULL: return 1;
        case VAL_BOOL: return a.b == b.b;
        case VAL_INT: return a.i == b.i;
        case VAL_FLOAT: return a.f == b.f;
        case VAL_STRING: return strcmp(a.s ? a.s : "", b.s ? b.s : "") == 0;
        case VAL_ARRAY: return a.a == b.a;
        default: return 0;
    }
}

static Value n_abs(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    if (argv[0].type == VAL_FLOAT) return value_float(fabs(argv[0].f));
    if (argv[0].type == VAL_INT) return value_int(argv[0].i < 0 ? -argv[0].i : argv[0].i);
    if (argv[0].type == VAL_BOOL) return value_int(argv[0].b ? 1 : 0);
    Value v = value_to_int(argv[0]);
    return value_int(v.i < 0 ? -v.i : v.i);
}

static Value n_min(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_int(0);
    if (argv[0].type == VAL_FLOAT || argv[1].type == VAL_FLOAT) {
        double a = value_as_double(argv[0]);
        double b = value_as_double(argv[1]);
        return value_float(a < b ? a : b);
    }
    int a = value_to_int(argv[0]).i;
    int b = value_to_int(argv[1]).i;
    return value_int(a < b ? a : b);
}

static Value n_max(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_int(0);
    if (argv[0].type == VAL_FLOAT || argv[1].type == VAL_FLOAT) {
        double a = value_as_double(argv[0]);
        double b = value_as_double(argv[1]);
        return value_float(a > b ? a : b);
    }
    int a = value_to_int(argv[0]).i;
    int b = value_to_int(argv[1]).i;
    return value_int(a > b ? a : b);
}

static Value n_round(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(round(v));
}

static Value n_floor(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(floor(v));
}

static Value n_ceil(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(ceil(v));
}

static Value n_sqrt(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(sqrt(v));
}

static Value n_exp(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(exp(v));
}

static Value n_log(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(log(v));
}

static Value n_sin(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(sin(value_as_double(argv[0])));
}

static Value n_cos(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(cos(value_as_double(argv[0])));
}

static Value n_tan(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(tan(value_as_double(argv[0])));
}

static Value n_asin(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(asin(value_as_double(argv[0])));
}

static Value n_acos(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(acos(value_as_double(argv[0])));
}

static Value n_atan(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    return value_float(atan(value_as_double(argv[0])));
}

static Value n_atan2(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_float(0.0);
    double y = value_as_double(argv[0]);
    double x = value_as_double(argv[1]);
    return value_float(atan2(y, x));
}

static Value n_rand(Env *env, int argc, Value *argv){
    (void)env;
    if (argc == 0) return value_int(rand());
    if (argc == 1) {
        int max = value_to_int(argv[0]).i;
        if (max <= 0) return value_int(0);
        return value_int(rand() % (max + 1));
    }
    if (argc >= 2) {
        int min = value_to_int(argv[0]).i;
        int max = value_to_int(argv[1]).i;
        if (min > max) {
            int tmp = min;
            min = max;
            max = tmp;
        }
        int span = max - min;
        if (span <= 0) return value_int(min);
        return value_int(min + (rand() % (span + 1)));
    }
    return value_int(0);
}

static Value n_srand(Env *env, int argc, Value *argv){
    (void)env;
    if (argc == 0) {
        srand((unsigned int)time(NULL));
    } else {
        srand((unsigned int)value_to_int(argv[0]).i);
    }
    return value_void();
}

static Value n_clamp(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 3) return value_int(0);
    double v = value_as_double(argv[0]);
    double lo = value_as_double(argv[1]);
    double hi = value_as_double(argv[2]);
    if (lo > hi) {
        double tmp = lo;
        lo = hi;
        hi = tmp;
    }
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    if (argv[0].type == VAL_INT && argv[1].type == VAL_INT && argv[2].type == VAL_INT) {
        return value_int((int)v);
    }
    return value_float(v);
}

static Value n_pi(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_float(0.0);
    return value_float(3.14159265358979323846);
}

static Value n_sign(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    double v = value_as_double(argv[0]);
    if (v > 0.0) return value_int(1);
    if (v < 0.0) return value_int(-1);
    return value_int(0);
}

static Value n_deg2rad(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(v * (3.14159265358979323846 / 180.0));
}

static Value n_rad2deg(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    double v = value_as_double(argv[0]);
    return value_float(v * (180.0 / 3.14159265358979323846));
}

static Value n_key_exists(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[1].type != VAL_ARRAY || !argv[1].a) return value_bool(0);
    Array *a = argv[1].a;
    if (argv[0].type == VAL_STRING) {
        const char *ks = argv[0].s ? argv[0].s : "";
        for (int i = 0; i < a->size; i++) {
            if (a->entries[i].key.type == KEY_STRING &&
                strcmp(a->entries[i].key.s ? a->entries[i].key.s : "", ks) == 0) {
                return value_bool(1);
            }
        }
        return value_bool(0);
    }
    int ki = value_to_int(argv[0]).i;
    for (int i = 0; i < a->size; i++) {
        if (a->entries[i].key.type == KEY_INT && a->entries[i].key.i == ki) {
            return value_bool(1);
        }
    }
    return value_bool(0);
}

static Value n_values(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) return out;
    Array *a = argv[0].a;
    for (int i = 0; i < a->size; i++) {
        array_set(out.a, key_int(i), value_copy(a->entries[i].value));
    }
    return out;
}

static Value n_in_array(Env *env, int argc, Value *argv){
    (void)env;
    int strict = 1;
    if (argc == 3) {
        if (argv[2].type != VAL_BOOL) return value_bool(0);
        strict = argv[2].b;
    }
    if (argc != 2 && argc != 3) return value_bool(0);
    if (argv[1].type != VAL_ARRAY || !argv[1].a) return value_bool(0);
    Array *a = argv[1].a;
    for (int i = 0; i < a->size; i++) {
        int eq = strict ? strict_equal_native(argv[0], a->entries[i].value)
                        : weak_equal_native(argv[0], a->entries[i].value);
        if (eq) {
            return value_bool(1);
        }
    }
    return value_bool(0);
}

static Value n_push(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_ARRAY || !argv[0].a) return value_int(0);
    Array *a = argv[0].a;
    int idx = array_next_index(a);
    array_set(a, key_int(idx), value_copy(argv[1]));
    return value_int(a->size);
}

static Value n_pop(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) return value_undefined();
    Array *a = argv[0].a;
    if (a->size == 0) return value_undefined();
    Value out = a->entries[a->size - 1].value;
    key_free_local(a->entries[a->size - 1].key);
    a->size--;
    return out;
}

static Value n_shift(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) return value_undefined();
    Array *a = argv[0].a;
    if (a->size == 0) return value_undefined();
    Value out = a->entries[0].value;
    key_free_local(a->entries[0].key);
    if (a->size > 1) {
        memmove(&a->entries[0], &a->entries[1], (size_t)(a->size - 1) * sizeof(ArrayEntry));
    }
    a->size--;
    reindex_numeric_keys(a);
    return out;
}

static Value n_unshift(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_ARRAY || !argv[0].a) return value_int(0);
    Array *a = argv[0].a;
    if (a->capacity < a->size + 1) {
        int cap = a->capacity ? a->capacity : 8;
        while (cap < a->size + 1) cap *= 2;
        ArrayEntry *ne = (ArrayEntry*)realloc(a->entries, (size_t)cap * sizeof(ArrayEntry));
        if (!ne) return value_int(a->size);
        a->entries = ne;
        a->capacity = cap;
    }
    if (a->size > 0) {
        memmove(&a->entries[1], &a->entries[0], (size_t)a->size * sizeof(ArrayEntry));
    }
    a->entries[0].key = key_int(0);
    a->entries[0].value = value_copy(argv[1]);
    a->size++;
    reindex_numeric_keys(a);
    return value_int(a->size);
}

static Value n_merge(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc != 2 || argv[0].type != VAL_ARRAY || argv[1].type != VAL_ARRAY) return out;
    Array *a = argv[0].a;
    Array *b = argv[1].a;
    int next = 0;
    if (a) {
        for (int i = 0; i < a->size; i++) {
            if (a->entries[i].key.type == KEY_STRING) {
                array_set(out.a, key_string(a->entries[i].key.s), value_copy(a->entries[i].value));
            } else {
                array_set(out.a, key_int(next++), value_copy(a->entries[i].value));
            }
        }
    }
    if (b) {
        for (int i = 0; i < b->size; i++) {
            if (b->entries[i].key.type == KEY_STRING) {
                array_set(out.a, key_string(b->entries[i].key.s), value_copy(b->entries[i].value));
            } else {
                array_set(out.a, key_int(next++), value_copy(b->entries[i].value));
            }
        }
    }
    return out;
}

static Value n_slice(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc < 2 || argv[0].type != VAL_ARRAY || !argv[0].a) return out;
    Array *a = argv[0].a;
    int count = a->size;
    int start = value_to_int(argv[1]).i;
    int len = (argc >= 3) ? value_to_int(argv[2]).i : (count - start);
    if (start < 0) start = 0;
    if (start > count) start = count;
    if (len < 0) len = 0;
    if (start + len > count) len = count - start;
    int next = 0;
    for (int i = start; i < start + len; i++) {
        if (a->entries[i].key.type == KEY_STRING) {
            array_set(out.a, key_string(a->entries[i].key.s), value_copy(a->entries[i].value));
        } else {
            array_set(out.a, key_int(next++), value_copy(a->entries[i].value));
        }
    }
    return out;
}

static Value n_splice(Env *env, int argc, Value *argv){
    (void)env;
    Value removed = value_array();
    if (argc < 2 || argv[0].type != VAL_ARRAY || !argv[0].a) return removed;
    Array *a = argv[0].a;
    int count = a->size;
    int start = value_to_int(argv[1]).i;
    int len = (argc >= 3) ? value_to_int(argv[2]).i : (count - start);
    if (start < 0) start = 0;
    if (start > count) start = count;
    if (len < 0) len = 0;
    if (start + len > count) len = count - start;

    int ridx = 0;
    for (int i = start; i < start + len; i++) {
        array_set(removed.a, key_int(ridx++), value_copy(a->entries[i].value));
    }

    Value temp = value_array();
    int next = 0;
    for (int i = 0; i < start; i++) {
        if (a->entries[i].key.type == KEY_STRING) {
            array_set(temp.a, key_string(a->entries[i].key.s), value_copy(a->entries[i].value));
        } else {
            array_set(temp.a, key_int(next++), value_copy(a->entries[i].value));
        }
    }
    if (argc >= 4) {
        if (argv[3].type == VAL_ARRAY && argv[3].a) {
            for (int i = 0; i < argv[3].a->size; i++) {
                array_set(temp.a, key_int(next++), value_copy(argv[3].a->entries[i].value));
            }
        } else {
            array_set(temp.a, key_int(next++), value_copy(argv[3]));
        }
    }
    for (int i = start + len; i < count; i++) {
        if (a->entries[i].key.type == KEY_STRING) {
            array_set(temp.a, key_string(a->entries[i].key.s), value_copy(a->entries[i].value));
        } else {
            array_set(temp.a, key_int(next++), value_copy(a->entries[i].value));
        }
    }

    for (int i = 0; i < a->size; i++) {
        key_free_local(a->entries[i].key);
        value_free(a->entries[i].value);
    }
    free(a->entries);
    a->entries = temp.a->entries;
    a->size = temp.a->size;
    a->capacity = temp.a->capacity;
    gc_unregister_array(temp.a);
    free(temp.a);
    return removed;
}

static Value n_reverse(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) return out;
    Array *a = argv[0].a;
    int next = 0;
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->entries[i].key.type == KEY_STRING) {
            array_set(out.a, key_string(a->entries[i].key.s), value_copy(a->entries[i].value));
        } else {
            array_set(out.a, key_int(next++), value_copy(a->entries[i].value));
        }
    }
    return out;
}

static int value_compare_native(Value a, Value b) {
    if (value_is_number(a) && value_is_number(b)) {
        double da = value_as_double(a);
        double db = value_as_double(b);
        if (da < db) return -1;
        if (da > db) return 1;
        return 0;
    }
    Value sa = value_to_string(a);
    Value sb = value_to_string(b);
    int cmp = strcmp(sa.s ? sa.s : "", sb.s ? sb.s : "");
    value_free(sa);
    value_free(sb);
    if (cmp < 0) return -1;
    if (cmp > 0) return 1;
    return 0;
}

typedef struct {
    Key key;
    Value value;
} SortEntry;

static int g_sort_desc = 0;
static int g_sort_by_key = 0;

static int key_compare_native(Key a, Key b) {
    if (a.type == KEY_INT && b.type == KEY_INT) {
        if (a.i < b.i) return -1;
        if (a.i > b.i) return 1;
        return 0;
    }
    const char *sa = NULL;
    const char *sb = NULL;
    char bufa[32];
    char bufb[32];
    if (a.type == KEY_STRING) {
        sa = a.s ? a.s : "";
    } else {
        snprintf(bufa, sizeof(bufa), "%d", a.i);
        sa = bufa;
    }
    if (b.type == KEY_STRING) {
        sb = b.s ? b.s : "";
    } else {
        snprintf(bufb, sizeof(bufb), "%d", b.i);
        sb = bufb;
    }
    int cmp = strcmp(sa, sb);
    if (cmp < 0) return -1;
    if (cmp > 0) return 1;
    return 0;
}

static int qsort_entry_cmp(const void *pa, const void *pb) {
    const SortEntry *a = (const SortEntry *)pa;
    const SortEntry *b = (const SortEntry *)pb;
    int cmp = g_sort_by_key
        ? key_compare_native(a->key, b->key)
        : value_compare_native(a->value, b->value);
    return g_sort_desc ? -cmp : cmp;
}

static Value n_sort_common(Env *env, int argc, Value *argv, int by_key, int desc, int preserve_keys){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) return value_bool(0);
    Array *a = argv[0].a;
    int count = a->size;
    if (count <= 1) return value_bool(1);

    SortEntry *entries = (SortEntry *)calloc((size_t)count, sizeof(SortEntry));
    if (!entries) return value_bool(0);
    for (int i = 0; i < count; i++) {
        entries[i].key = key_copy_local(a->entries[i].key);
        entries[i].value = value_copy(a->entries[i].value);
    }

    g_sort_by_key = by_key;
    g_sort_desc = desc;
    qsort(entries, (size_t)count, sizeof(SortEntry), qsort_entry_cmp);

    for (int i = 0; i < count; i++) {
        key_free_local(a->entries[i].key);
        value_free(a->entries[i].value);
    }
    free(a->entries);
    a->entries = NULL;
    a->size = 0;
    a->capacity = 0;

    for (int i = 0; i < count; i++) {
        Key k;
        if (preserve_keys) {
            k = (entries[i].key.type == KEY_STRING)
                ? key_string(entries[i].key.s ? entries[i].key.s : "")
                : key_int(entries[i].key.i);
        } else {
            k = key_int(i);
        }
        array_set(a, k, entries[i].value);
        key_free_local(entries[i].key);
    }
    free(entries);
    return value_bool(1);
}

static Value n_sort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 0, 0, 0);
}

static Value n_rsort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 0, 1, 0);
}

static Value n_asort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 0, 0, 1);
}

static Value n_arsort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 0, 1, 1);
}

static Value n_ksort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 1, 0, 1);
}

static Value n_krsort(Env *env, int argc, Value *argv){
    return n_sort_common(env, argc, argv, 1, 1, 1);
}

static Value n_trim(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && is_trim_space((unsigned char)s[start])) start++;
    if (start == len) return value_string("");
    size_t end = len;
    while (end > start && is_trim_space((unsigned char)s[end - 1])) end--;
    return value_string_n(s + start, end - start);
}

static Value n_ltrim(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && is_trim_space((unsigned char)s[start])) start++;
    return value_string_n(s + start, len - start);
}

static Value n_rtrim(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    size_t end = len;
    while (end > 0 && is_trim_space((unsigned char)s[end - 1])) end--;
    return value_string_n(s, end);
}

static Value n_ucfirst(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    char *buf = (char*)malloc(len + 1);
    if (!buf) return value_string("");
    memcpy(buf, s, len + 1);
    if (len > 0) buf[0] = (char)toupper((unsigned char)buf[0]);
    Value out = value_string(buf);
    free(buf);
    return out;
}

static Value n_strtolower(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    char *buf = (char*)malloc(len + 1);
    if (!buf) return value_string("");
    for (size_t i = 0; i < len; i++) buf[i] = (char)tolower((unsigned char)s[i]);
    buf[len] = '\0';
    Value out = value_string(buf);
    free(buf);
    return out;
}

static Value n_strtoupper(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    char *buf = (char*)malloc(len + 1);
    if (!buf) return value_string("");
    for (size_t i = 0; i < len; i++) buf[i] = (char)toupper((unsigned char)s[i]);
    buf[len] = '\0';
    Value out = value_string(buf);
    free(buf);
    return out;
}

static Value n_strpos(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_undefined();
    }
    const char *hay = argv[0].s ? argv[0].s : "";
    const char *needle = argv[1].s ? argv[1].s : "";
    const char *pos = strstr(hay, needle);
    if (!pos) return value_undefined();
    return value_int((int)(pos - hay));
}

static Value n_strrpos(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_undefined();
    }
    const char *hay = argv[0].s ? argv[0].s : "";
    const char *needle = argv[1].s ? argv[1].s : "";
    const char *last = NULL;
    const char *p = hay;
    while ((p = strstr(p, needle)) != NULL) {
        last = p;
        p++;
    }
    if (!last) return value_undefined();
    return value_int((int)(last - hay));
}

static Value n_strcmp(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_int(0);
    }
    const char *a = argv[0].s ? argv[0].s : "";
    const char *b = argv[1].s ? argv[1].s : "";
    return value_int(strcmp(a, b));
}

static Value n_str_replace(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 3 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING ||
        argv[2].type != VAL_STRING) {
        return value_string("");
    }
    const char *needle = argv[0].s ? argv[0].s : "";
    const char *repl = argv[1].s ? argv[1].s : "";
    const char *hay = argv[2].s ? argv[2].s : "";

    if (*needle == '\0') return value_string(hay);

    char *buf = NULL;
    size_t cap = 0;
    size_t len = 0;
    const char *cur = hay;
    while (1) {
        const char *pos = strstr(cur, needle);
        if (!pos) break;
        size_t seg_len = (size_t)(pos - cur);
        buf_append(&buf, &cap, &len, cur, seg_len);
        buf_append(&buf, &cap, &len, repl, strlen(repl));
        cur = pos + strlen(needle);
    }
    buf_append(&buf, &cap, &len, cur, strlen(cur));

    Value out = value_string(buf ? buf : "");
    free(buf);
    return out;
}

static Value n_str_contains(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const char *hay = argv[0].s ? argv[0].s : "";
    const char *needle = argv[1].s ? argv[1].s : "";
    if (*needle == '\0') return value_bool(1);
    return value_bool(strstr(hay, needle) != NULL);
}

static Value n_starts_with(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const char *hay = argv[0].s ? argv[0].s : "";
    const char *needle = argv[1].s ? argv[1].s : "";
    size_t nlen = strlen(needle);
    size_t hlen = strlen(hay);
    if (nlen == 0) return value_bool(1);
    if (nlen > hlen) return value_bool(0);
    return value_bool(strncmp(hay, needle, nlen) == 0);
}

static Value n_ends_with(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const char *hay = argv[0].s ? argv[0].s : "";
    const char *needle = argv[1].s ? argv[1].s : "";
    size_t nlen = strlen(needle);
    size_t hlen = strlen(hay);
    if (nlen == 0) return value_bool(1);
    if (nlen > hlen) return value_bool(0);
    return value_bool(strncmp(hay + hlen - nlen, needle, nlen) == 0);
}

static Value n_lx_info(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_string("");

    char *buf = NULL;
    size_t cap = 0;
    size_t len = 0;
    buf_append(&buf, &cap, &len, "Lx ", 3);
    buf_append(&buf, &cap, &len, LX_VERSION_STRING, strlen(LX_VERSION_STRING));
    buf_append(&buf, &cap, &len, "\n", 1);
    buf_append(&buf, &cap, &len, "extensions: ", 12);
    int count = lx_extension_count();
    if (count == 0) {
        buf_append(&buf, &cap, &len, "(none)", 6);
    } else {
        for (int i = 0; i < count; i++) {
            const char *name = lx_extension_name(i);
            if (!name) continue;
            if (i > 0) buf_append(&buf, &cap, &len, ", ", 2);
            buf_append(&buf, &cap, &len, name, strlen(name));
        }
    }
    buf_append(&buf, &cap, &len, "\n", 1);

    Value out = value_string(buf ? buf : "");
    free(buf);
    return out;
}

static Value n_get_type(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_string("undefined");
    switch (argv[0].type){
        case VAL_UNDEFINED: return value_string("undefined");
        case VAL_VOID:      return value_string("void");
        case VAL_NULL:      return value_string("null");
        case VAL_BOOL:      return value_string("bool");
        case VAL_INT:       return value_string("int");
        case VAL_FLOAT:     return value_string("float");
        case VAL_STRING:    return value_string("string");
        case VAL_ARRAY:     return value_string("array");
        default:            return value_string("undefined");
    }
}

static Value make_is(ValueType t, int argc, Value *argv){
    if (argc != 1) return value_bool(0);
    return value_bool(argv[0].type == t);
}

static Value n_is_null(Env *env,int a,Value *v){ (void)env; return make_is(VAL_NULL,a,v); }
static Value n_is_bool(Env *env,int a,Value *v){ (void)env; return make_is(VAL_BOOL,a,v); }
static Value n_is_int(Env *env,int a,Value *v){ (void)env; return make_is(VAL_INT,a,v); }
static Value n_is_float(Env *env,int a,Value *v){ (void)env; return make_is(VAL_FLOAT,a,v); }
static Value n_is_string(Env *env,int a,Value *v){ (void)env; return make_is(VAL_STRING,a,v); }
static Value n_is_array(Env *env,int a,Value *v){ (void)env; return make_is(VAL_ARRAY,a,v); }
static Value n_is_defined(Env *env,int a,Value *v){
    (void)env;
    if (a != 1) return value_bool(0);
    return value_bool(v[0].type != VAL_UNDEFINED);
}
static Value n_is_undefined(Env *env,int a,Value *v){ (void)env; return make_is(VAL_UNDEFINED,a,v); }
static Value n_is_void(Env *env,int a,Value *v){ (void)env; return make_is(VAL_VOID,a,v); }

static Value n_pow(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_null();
    Value a = value_to_float(argv[0]);
    Value b = value_to_float(argv[1]);
    double r = pow(a.f, b.f);
    return value_float(r);
}

static Value n_ord(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    if (argv[0].type != VAL_STRING || !argv[0].s || argv[0].s[0] == '\0') {
        return value_int(0);
    }
    return value_int((unsigned char)argv[0].s[0]);
}

static Value n_split(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_array();
    if (argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) return value_array();

    const char *delim = argv[0].s ? argv[0].s : "";
    const char *s = argv[1].s ? argv[1].s : "";
    size_t dlen = strlen(delim);

    Value out = value_array();
    int idx = 0;

    if (dlen == 0) {
        array_set(out.a, key_int(idx++), value_string(s));
        return out;
    }

    const char *cur = s;
    while (1) {
        const char *pos = strstr(cur, delim);
        if (!pos) break;
        size_t seg_len = (size_t)(pos - cur);
        array_set(out.a, key_int(idx++), value_string_n(cur, seg_len));
        cur = pos + dlen;
    }
    array_set(out.a, key_int(idx++), value_string(cur));
    return out;
}

static Value n_join(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_string("");

    Array *arr = NULL;
    const char *sep = "";

    if (argv[0].type == VAL_ARRAY && argv[1].type == VAL_STRING) {
        arr = argv[0].a;
        sep = argv[1].s ? argv[1].s : "";
    } else if (argv[0].type == VAL_STRING && argv[1].type == VAL_ARRAY) {
        sep = argv[0].s ? argv[0].s : "";
        arr = argv[1].a;
    } else {
        return value_string("");
    }

    if (!arr) return value_string("");

    char *buf = NULL;
    size_t cap = 0;
    size_t len = 0;
    for (int i = 0; i < arr->size; i++) {
        if (i > 0 && *sep) {
            buf_append(&buf, &cap, &len, sep, strlen(sep));
        } else if (i > 0 && !*sep) {
            /* no-op */
        }
        Value sv = value_to_string(arr->entries[i].value);
        const char *s = sv.s ? sv.s : "";
        buf_append(&buf, &cap, &len, s, strlen(s));
        value_free(sv);
    }

    Value out = value_string(buf ? buf : "");
    free(buf);
    return out;
}

static int parse_binary_int(const char *s, int *out) {
    int v = 0;
    if (!s || !*s) return 0;
    for (const char *p = s; *p; p++) {
        if (*p != '0' && *p != '1') return 0;
        v = (v << 1) | (*p - '0');
    }
    *out = v;
    return 1;
}

static int parse_int_string(const char *s, int *out) {
    if (!s || !*s) return 0;
    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') sign = -1;
        s++;
    }
    if (!*s) return 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        char *end = NULL;
        long v = strtol(s, &end, 16);
        if (end && *end == '\0') { *out = (int)v * sign; return 1; }
        return 0;
    }
    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        int v = 0;
        if (parse_binary_int(s + 2, &v)) { *out = v * sign; return 1; }
        return 0;
    }
    if (s[0] == '0' && isdigit((unsigned char)s[1])) {
        int octal_ok = 1;
        for (const char *p = s + 1; *p; p++) {
            if (*p < '0' || *p > '7') { octal_ok = 0; break; }
        }
        if (octal_ok) {
            char *end = NULL;
            long v = strtol(s, &end, 8);
            if (end && *end == '\0') { *out = (int)v * sign; return 1; }
            return 0;
        }
    }
    {
        char *end = NULL;
        long v = strtol(s, &end, 10);
        if (end && *end == '\0') { *out = (int)v * sign; return 1; }
    }
    return 0;
}

static int parse_float_string(const char *s, double *out) {
    if (!s || !*s) return 0;
    const char *orig = s;
    if (*s == '+' || *s == '-') s++;
    if (!*s) return 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        int v = 0;
        if (parse_int_string(orig, &v)) {
            *out = (double)v;
            return 1;
        }
        return 0;
    }
    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        int v = 0;
        if (parse_int_string(orig, &v)) {
            *out = (double)v;
            return 1;
        }
        return 0;
    }
    if (s[0] == '0' && isdigit((unsigned char)s[1])) {
        int v = 0;
        if (parse_int_string(orig, &v)) {
            *out = (double)v;
            return 1;
        }
    }

    {
        char *end = NULL;
        double v = strtod(orig, &end);
        if (end && *end == '\0') { *out = v; return 1; }
    }
    return 0;
}

static Value n_int(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_int(0);
    if (argv[0].type == VAL_STRING) {
        int v = 0;
        if (parse_int_string(argv[0].s ? argv[0].s : "", &v)) {
            return value_int(v);
        }
    }
    return value_to_int(argv[0]);
}

static Value n_float(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_float(0.0);
    if (argv[0].type == VAL_STRING) {
        double v = 0.0;
        if (parse_float_string(argv[0].s ? argv[0].s : "", &v)) {
            return value_float(v);
        }
    }
    return value_to_float(argv[0]);
}

static Value n_str(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_string("");
    return value_to_string(argv[0]);
}

static void buf_append(char **buf, size_t *cap, size_t *len, const char *s, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 64 : *cap;
        while (ncap < *len + n + 1) ncap *= 2;
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return;
        *buf = nb;
        *cap = ncap;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
}

static int is_flag_char(char c) {
    return (c == '-' || c == '+' || c == ' ' || c == '#' || c == '0');
}

static int is_length_char(char c) {
    return (c == 'h' || c == 'l' || c == 'L' || c == 'z' || c == 't');
}

static Value n_sprintf(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1) return value_string("");

    Value fmtv = value_to_string(argv[0]);
    const char *fmt = fmtv.s ? fmtv.s : "";
    char *out = NULL;
    size_t cap = 0;
    size_t len = 0;
    int argi = 1;

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            buf_append(&out, &cap, &len, &fmt[i], 1);
            continue;
        }

        if (fmt[i + 1] == '%') {
            buf_append(&out, &cap, &len, "%", 1);
            i++;
            continue;
        }

        size_t start = i;
        i++;

        while (is_flag_char(fmt[i])) i++;

        if (fmt[i] == '*') {
            buf_append(&out, &cap, &len, &fmt[start], 1);
            i = start;
            continue;
        }
        while (isdigit((unsigned char)fmt[i])) i++;

        if (fmt[i] == '.') {
            i++;
            if (fmt[i] == '*') {
                buf_append(&out, &cap, &len, &fmt[start], 1);
                i = start;
                continue;
            }
            while (isdigit((unsigned char)fmt[i])) i++;
        }

        if (is_length_char(fmt[i])) {
            i++;
            if ((fmt[i - 1] == 'h' && fmt[i] == 'h') ||
                (fmt[i - 1] == 'l' && fmt[i] == 'l')) {
                i++;
            }
        }

        char spec = fmt[i];
        if (spec == '\0') break;

        size_t frag_len = (i - start) + 1;
        char frag[64];
        if (frag_len >= sizeof(frag)) {
            buf_append(&out, &cap, &len, &fmt[start], frag_len);
            continue;
        }
        memcpy(frag, &fmt[start], frag_len);
        frag[frag_len] = '\0';

        if (argi >= argc) {
            buf_append(&out, &cap, &len, frag, frag_len);
            continue;
        }

        if (spec == 's') {
            Value sv = value_to_string(argv[argi++]);
            const char *s = sv.s ? sv.s : "";
            int n = snprintf(NULL, 0, frag, s);
            if (n > 0) {
                char *tmp = (char *)malloc((size_t)n + 1);
                if (tmp) {
                    snprintf(tmp, (size_t)n + 1, frag, s);
                    buf_append(&out, &cap, &len, tmp, (size_t)n);
                    free(tmp);
                }
            }
            value_free(sv);
            continue;
        }
        if (spec == 'c') {
            int ch = 0;
            int empty_string = 0;
            if (argv[argi].type == VAL_STRING && argv[argi].s && argv[argi].s[0] != '\0') {
                ch = (unsigned char)argv[argi].s[0];
                argi++;
            } else if (argv[argi].type == VAL_STRING && argv[argi].s && argv[argi].s[0] == '\0') {
                empty_string = 1;
                argi++;
            } else {
                Value iv = value_to_int(argv[argi++]);
                ch = iv.i;
                value_free(iv);
            }
            if (empty_string) {
                continue;
            }
            int n = snprintf(NULL, 0, frag, ch);
            if (n > 0) {
                char *tmp = (char *)malloc((size_t)n + 1);
                if (tmp) {
                    snprintf(tmp, (size_t)n + 1, frag, ch);
                    buf_append(&out, &cap, &len, tmp, (size_t)n);
                    free(tmp);
                }
            }
            continue;
        }
        if (spec == 'd' || spec == 'i' || spec == 'u' ||
            spec == 'x' || spec == 'X' || spec == 'o') {
            Value iv = value_to_int(argv[argi++]);
            long long v = (long long)iv.i;
            unsigned long long uv = (unsigned long long)iv.i;
            value_free(iv);
            int n = (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'o')
                ? snprintf(NULL, 0, frag, uv)
                : snprintf(NULL, 0, frag, v);
            if (n > 0) {
                char *tmp = (char *)malloc((size_t)n + 1);
                if (tmp) {
                    if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'o')
                        snprintf(tmp, (size_t)n + 1, frag, uv);
                    else
                        snprintf(tmp, (size_t)n + 1, frag, v);
                    buf_append(&out, &cap, &len, tmp, (size_t)n);
                    free(tmp);
                }
            }
            continue;
        }
        if (spec == 'f' || spec == 'F' || spec == 'e' || spec == 'E' ||
            spec == 'g' || spec == 'G') {
            Value fv = value_to_float(argv[argi++]);
            double d = fv.f;
            value_free(fv);
            int n = snprintf(NULL, 0, frag, d);
            if (n > 0) {
                char *tmp = (char *)malloc((size_t)n + 1);
                if (tmp) {
                    snprintf(tmp, (size_t)n + 1, frag, d);
                    buf_append(&out, &cap, &len, tmp, (size_t)n);
                    free(tmp);
                }
            }
            continue;
        }

        buf_append(&out, &cap, &len, frag, frag_len);
    }

    value_free(fmtv);
    Value outv = value_string(out ? out : "");
    free(out);
    return outv;
}

static Value n_chr(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_string("");
    Value vi = value_to_int(argv[0]);
    int code = vi.i;
    value_free(vi);
    if (code < 0) code = 0;
    if (code > 255) code = 255;
    char buf[2];
    buf[0] = (char)(unsigned char)code;
    buf[1] = '\0';
    return value_string(buf);
}

static Value n_printf(Env *env, int argc, Value *argv){
    Value s = n_sprintf(env, argc, argv);
    Value out = n_print(env, 1, &s);
    value_free(s);
    return out;
}

static Value n_array_keys(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_ARRAY || !argv[0].a) {
        return value_array();
    }
    Value out = value_array();
    int idx = 0;
    for (int i = 0; i < argv[0].a->size; i++) {
        ArrayEntry *e = &argv[0].a->entries[i];
        Value keyv = (e->key.type == KEY_STRING) ? value_string(e->key.s) : value_int(e->key.i);
        array_set(out.a, key_int(idx++), keyv);
    }
    return out;
}

void install_stdlib(void){
    register_function("print",   n_print);
    register_function("print_r", n_print_r);
    register_function("var_dump", n_var_dump);
#if LX_ENABLE_INCLUDE
    register_function("include", n_include);
    register_function("include_once", n_include_once);
#endif
    register_function("abs",     n_abs);
    register_function("min",     n_min);
    register_function("max",     n_max);
    register_function("round",   n_round);
    register_function("floor",   n_floor);
    register_function("ceil",    n_ceil);
    register_function("strlen",  n_strlen);
    register_function("base64_encode", n_base64_encode);
    register_function("base64_decode", n_base64_decode);
    register_function("crc32",   n_crc32);
    register_function("crc32u",  n_crc32u);
    register_function("count",   n_count);
    register_function("substr",  n_substr);
    register_function("trim",    n_trim);
    register_function("ltrim",   n_ltrim);
    register_function("rtrim",   n_rtrim);
    register_function("ucfirst", n_ucfirst);
    register_function("strtolower", n_strtolower);
    register_function("lower", n_strtolower);
    register_function("strtoupper", n_strtoupper);
    register_function("upper", n_strtoupper);
    register_function("strpos",  n_strpos);
    register_function("strrpos", n_strrpos);
    register_function("strcmp",  n_strcmp);
    register_function("str_replace", n_str_replace);
    register_function("str_contains", n_str_contains);
    register_function("starts_with", n_starts_with);
    register_function("ends_with", n_ends_with);
    register_function("lxinfo", n_lx_info);
    register_function("type",n_get_type);

    register_function("is_null",   n_is_null);
    register_function("is_bool",   n_is_bool);
    register_function("is_int",    n_is_int);
    register_function("is_float",  n_is_float);
    register_function("is_string", n_is_string);
    register_function("is_array",  n_is_array);
    register_function("is_defined",   n_is_defined);
    register_function("is_undefined", n_is_undefined);
    register_function("is_void",      n_is_void);

    register_function("pow", n_pow);
    register_function("sqrt", n_sqrt);
    register_function("exp", n_exp);
    register_function("log", n_log);
    register_function("sin", n_sin);
    register_function("cos", n_cos);
    register_function("tan", n_tan);
    register_function("asin", n_asin);
    register_function("acos", n_acos);
    register_function("atan", n_atan);
    register_function("atan2", n_atan2);
    register_function("rand", n_rand);
    register_function("srand", n_srand);
    register_function("clamp", n_clamp);
    register_function("pi", n_pi);
    register_function("sign", n_sign);
    register_function("deg2rad", n_deg2rad);
    register_function("rad2deg", n_rad2deg);
    register_function("ord", n_ord);
    register_function("chr", n_chr);
    register_function("sprintf", n_sprintf);
    register_function("printf", n_printf);
    register_function("keys", n_array_keys);
    register_function("key_exists", n_key_exists);
    register_function("values", n_values);
    register_function("in_array", n_in_array);
    register_function("push", n_push);
    register_function("pop", n_pop);
    register_function("shift", n_shift);
    register_function("unshift", n_unshift);
    register_function("merge", n_merge);
    register_function("slice", n_slice);
    register_function("splice", n_splice);
    register_function("reverse", n_reverse);
    register_function("sort", n_sort);
    register_function("rsort", n_rsort);
    register_function("asort", n_asort);
    register_function("arsort", n_arsort);
    register_function("ksort", n_ksort);
    register_function("krsort", n_krsort);
    register_function("int", n_int);
    register_function("float", n_float);
    register_function("str", n_str);
    register_function("split", n_split);
    register_function("join", n_join);
    register_function("explode", n_split);
    register_function("implode", n_join);
}
