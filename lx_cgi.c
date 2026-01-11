/**
 * @file lx_cgi.c
 * @brief Minimal CGI wrapper for Lx with <?lx ... ?> blocks.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "natives.h"
#include "array.h"
#include "lx_ext.h"
#include "lx_error.h"

void register_fs_module(void);
void register_json_module(void);
void register_serializer_module(void);
void register_hex_module(void);
void register_blake2b_module(void);
void register_time_module(void);
void register_env_module(void);
void register_utf8_module(void);
void register_sqlite_module(void);

extern char **environ;

typedef struct {
    char **items;
    int count;
    int cap;
    char *content_type;
} HeaderList;

static HeaderList g_headers = {0};
static int g_headers_sent = 0;

static void headers_reset(void) {
    for (int i = 0; i < g_headers.count; i++) {
        free(g_headers.items[i]);
    }
    free(g_headers.items);
    g_headers.items = NULL;
    g_headers.count = 0;
    g_headers.cap = 0;
    free(g_headers.content_type);
    g_headers.content_type = NULL;
    g_headers_sent = 0;
}

static void headers_add(const char *line) {
    if (!line || !*line) return;
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncasecmp(p, "Content-Type:", 13) == 0) {
        p += 13;
        while (*p == ' ' || *p == '\t') p++;
        free(g_headers.content_type);
        g_headers.content_type = strdup(*p ? p : "text/html; charset=utf-8");
        return;
    }
    if (g_headers.count + 1 > g_headers.cap) {
        int cap = g_headers.cap ? g_headers.cap * 2 : 8;
        char **items = (char **)realloc(g_headers.items, (size_t)cap * sizeof(char *));
        if (!items) return;
        g_headers.items = items;
        g_headers.cap = cap;
    }
    g_headers.items[g_headers.count++] = strdup(p);
}

static void headers_send(void) {
    if (g_headers_sent) return;
    const char *ct = g_headers.content_type ? g_headers.content_type : "text/html; charset=utf-8";
    printf("Content-Type: %s\r\n", ct);
    for (int i = 0; i < g_headers.count; i++) {
        printf("%s\r\n", g_headers.items[i]);
    }
    printf("\r\n");
    g_headers_sent = 1;
}

static Value n_header(Env *env, int argc, Value *argv) {
    (void)env;
    if (argc < 1) return value_void();
    Value s = value_to_string(argv[0]);
    if (s.s && *s.s) {
        headers_add(s.s);
    }
    value_free(s);
    return value_void();
}

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

static int append_str(char **buf, size_t *len, size_t *cap, const char *s) {
    size_t n = strlen(s);
    if (*len + n + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 1024 : *cap;
        while (*len + n + 1 > ncap) ncap *= 2;
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return 0;
        *buf = nb;
        *cap = ncap;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
    return 1;
}

static int append_n(char **buf, size_t *len, size_t *cap, const char *s, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t ncap = (*cap == 0) ? 1024 : *cap;
        while (*len + n + 1 > ncap) ncap *= 2;
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return 0;
        *buf = nb;
        *cap = ncap;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
    return 1;
}

static int append_char(char **buf, size_t *len, size_t *cap, char c) {
    if (*len + 2 > *cap) {
        size_t ncap = (*cap == 0) ? 1024 : (*cap * 2);
        char *nb = (char *)realloc(*buf, ncap);
        if (!nb) return 0;
        *buf = nb;
        *cap = ncap;
    }
    (*buf)[(*len)++] = c;
    (*buf)[*len] = '\0';
    return 1;
}

static int append_text_print(char **out, size_t *len, size_t *cap, const char *s, size_t n) {
    if (!append_str(out, len, cap, "print(\"")) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\') { if (!append_str(out, len, cap, "\\\\")) return 0; }
        else if (c == '"') { if (!append_str(out, len, cap, "\\\"")) return 0; }
        else if (c == '$') { if (!append_str(out, len, cap, "\\$")) return 0; }
        else if (c == '\n') { if (!append_str(out, len, cap, "\\n")) return 0; }
        else if (c == '\r') { if (!append_str(out, len, cap, "\\r")) return 0; }
        else if (c == '\t') { if (!append_str(out, len, cap, "\\t")) return 0; }
        else { if (!append_char(out, len, cap, (char)c)) return 0; }
    }
    if (!append_str(out, len, cap, "\");\n")) return 0;
    return 1;
}

static char *compile_template(const char *src) {
    char *out = NULL;
    size_t len = 0, cap = 0;
    const char *p = src;
    while (*p) {
        const char *tag = strstr(p, "<?lx");
        if (!tag) {
            if (!append_text_print(&out, &len, &cap, p, strlen(p))) return NULL;
            return out;
        }
        if (tag > p) {
            if (!append_text_print(&out, &len, &cap, p, (size_t)(tag - p))) return NULL;
        }
        const char *code = tag + 4;
        const char *end = strstr(code, "?>");
        if (!end) {
            if (!append_str(&out, &len, &cap, code)) { free(out); return NULL; }
            if (!append_str(&out, &len, &cap, "\n")) { free(out); return NULL; }
            return out;
        }
        if (!append_n(&out, &len, &cap, code, (size_t)(end - code))) { free(out); return NULL; }
        if (!append_str(&out, &len, &cap, "\n")) { free(out); return NULL; }
        p = end + 2;
    }
    return out;
}

static int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *url_decode(const char *s) {
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '%' && i + 2 < len) {
            int h1 = hex_to_int(s[i + 1]);
            int h2 = hex_to_int(s[i + 2]);
            if (h1 >= 0 && h2 >= 0) {
                out[j++] = (char)((h1 << 4) | h2);
                i += 2;
                continue;
            }
        }
        if (s[i] == '+') {
            out[j++] = ' ';
        } else {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    return out;
}

static Value parse_kv(const char *qs) {
    Value out = value_array();
    if (!qs) return out;
    const char *p = qs;
    while (*p) {
        const char *amp = strchr(p, '&');
        size_t len = amp ? (size_t)(amp - p) : strlen(p);
        const char *eq = memchr(p, '=', len);
        size_t klen = eq ? (size_t)(eq - p) : len;
        size_t vlen = eq ? (len - klen - 1) : 0;
        char *kraw = (char *)malloc(klen + 1);
        char *vraw = (char *)malloc(vlen + 1);
        if (!kraw || !vraw) { free(kraw); free(vraw); return out; }
        memcpy(kraw, p, klen);
        kraw[klen] = '\0';
        if (eq && vlen > 0) memcpy(vraw, eq + 1, vlen);
        vraw[vlen] = '\0';
        char *k = url_decode(kraw);
        char *v = url_decode(vraw);
        free(kraw);
        free(vraw);
        if (!k || !v) { free(k); free(v); return out; }
        array_set(out.a, key_string(k), value_string(v));
        free(k);
        free(v);
        if (!amp) break;
        p = amp + 1;
    }
    return out;
}

static Value read_post(void) {
    const char *len_s = getenv("CONTENT_LENGTH");
    if (!len_s) return value_array();
    long len = strtol(len_s, NULL, 10);
    if (len <= 0 || len > 1024 * 1024) return value_array();
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) return value_array();
    size_t n = fread(buf, 1, (size_t)len, stdin);
    buf[n] = '\0';
    Value out = parse_kv(buf);
    free(buf);
    return out;
}

static void env_set_array(Env *env, const char *name, Value arr) {
    env_set(env, name, arr);
}

static Value merge_request(Value get, Value post) {
    Value out = value_array();
    if (get.type == VAL_ARRAY && get.a) {
        for (int i = 0; i < get.a->size; i++) {
            Key key = get.a->entries[i].key;
            Value v = value_copy(get.a->entries[i].value);
            if (key.type == KEY_STRING) array_set(out.a, key_string(key.s), v);
            else array_set(out.a, key_int(key.i), v);
        }
    }
    if (post.type == VAL_ARRAY && post.a) {
        for (int i = 0; i < post.a->size; i++) {
            Key key = post.a->entries[i].key;
            Value v = value_copy(post.a->entries[i].value);
            if (key.type == KEY_STRING) array_set(out.a, key_string(key.s), v);
            else array_set(out.a, key_int(key.i), v);
        }
    }
    return out;
}

static Value build_server_env(void) {
    Value out = value_array();
    for (char **env = environ; env && *env; env++) {
        const char *eq = strchr(*env, '=');
        if (!eq) continue;
        size_t klen = (size_t)(eq - *env);
        char *k = (char *)malloc(klen + 1);
        if (!k) continue;
        memcpy(k, *env, klen);
        k[klen] = '\0';
        const char *v = eq + 1;
        array_set(out.a, key_string(k), value_string(v));
        free(k);
    }
    return out;
}

static void install_std_env(Env *global) {
    Value get = parse_kv(getenv("QUERY_STRING"));
    Value post = value_array();
    const char *method = getenv("REQUEST_METHOD");
    const char *ctype = getenv("CONTENT_TYPE");
    if (method && strcmp(method, "POST") == 0 &&
        ctype && strstr(ctype, "application/x-www-form-urlencoded") == ctype) {
        post = read_post();
    }
    Value req = merge_request(get, post);
    Value server = build_server_env();

    env_set_array(global, "_GET", get);
    env_set_array(global, "_POST", post);
    env_set_array(global, "_REQUEST", req);
    env_set_array(global, "_SERVER", server);
}

static int run_script(const char *source, const char *filename) {
    Parser parser;
    lexer_init(&parser.lexer, source, filename);
    parser.current.type = TOK_ERROR;
    parser.previous.type = TOK_ERROR;
    lx_error_clear();

    AstNode *program = parse_program(&parser);
    if (lx_has_error() || !program) {
        lx_print_error(stderr);
        return 1;
    }

    Env *global = env_new(NULL);
    install_stdlib();
    register_function("header", n_header);
    register_fs_module();
    register_json_module();
    register_serializer_module();
    register_hex_module();
    register_blake2b_module();
    register_time_module();
    register_env_module();
    register_utf8_module();
    register_sqlite_module();
    lx_init_modules(global);
    install_std_env(global);

    EvalResult r = eval_program(program, global);
    if (lx_has_error()) {
        lx_print_error(stderr);
        value_free(r.value);
        env_free(global);
        return 1;
    }
    value_free(r.value);
    env_free(global);
    return 0;
}

static int is_wrapper_path(const char *path) {
    if (!path) return 0;
    const char *p = strstr(path, "/lx_cgi");
    if (!p) return 0;
    return p[7] == '\0' || p[7] == '/';
}

static char *script_from_wrapper_path(const char *script_filename) {
    const char *doc_root = getenv("DOCUMENT_ROOT");
    if (!script_filename || !doc_root) return NULL;
    const char *p = strstr(script_filename, "/lx_cgi/");
    if (!p || !p[8]) return NULL;
    const char *suffix = p + 8;
    size_t dlen = strlen(doc_root);
    size_t slen = strlen(suffix);
    char *out = (char *)malloc(dlen + 1 + slen + 1);
    if (!out) return NULL;
    memcpy(out, doc_root, dlen);
    out[dlen] = '/';
    memcpy(out + dlen + 1, suffix, slen + 1);
    return out;
}

static char *resolve_script_path(void) {
    const char *script_filename = getenv("SCRIPT_FILENAME");
    const char *path_translated = getenv("PATH_TRANSLATED");
    const char *path_info = getenv("PATH_INFO");
    const char *doc_root = getenv("DOCUMENT_ROOT");

    if (script_filename && *script_filename && !is_wrapper_path(script_filename)) {
        return strdup(script_filename);
    }
    if (script_filename && *script_filename) {
        char *from_wrapper = script_from_wrapper_path(script_filename);
        if (from_wrapper) return from_wrapper;
    }
    if (path_translated && *path_translated) {
        if (script_filename && path_info && doc_root && *doc_root) {
            size_t sflen = strlen(script_filename);
            if (strncmp(path_translated, script_filename, sflen) == 0) {
                size_t dlen = strlen(doc_root);
                size_t ilen = strlen(path_info);
                char *out = (char *)malloc(dlen + ilen + 1);
                if (!out) return NULL;
                memcpy(out, doc_root, dlen);
                memcpy(out + dlen, path_info, ilen + 1);
                return out;
            }
        }
        return strdup(path_translated);
    }
    if (path_info && doc_root && *doc_root) {
        size_t dlen = strlen(doc_root);
        size_t ilen = strlen(path_info);
        char *out = (char *)malloc(dlen + ilen + 1);
        if (!out) return NULL;
        memcpy(out, doc_root, dlen);
        memcpy(out + dlen, path_info, ilen + 1);
        return out;
    }
    if (script_filename && *script_filename) {
        return strdup(script_filename);
    }
    return NULL;
}

int main(void) {
    char *path = resolve_script_path();
    if (!path || !*path) {
        fprintf(stderr, "lx_cgi: missing script path\n");
        free(path);
        return 1;
    }

    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (*path) {
            (void)chdir(path);
        }
        *last_slash = '/';
    }

    const char *dbg = getenv("LX_CGI_DEBUG");
    if (dbg && *dbg) {
        printf("Content-Type: text/plain; charset=utf-8\r\n\r\n");
        printf("SCRIPT_FILENAME=%s\n", getenv("SCRIPT_FILENAME") ? getenv("SCRIPT_FILENAME") : "");
        printf("PATH_TRANSLATED=%s\n", getenv("PATH_TRANSLATED") ? getenv("PATH_TRANSLATED") : "");
        printf("PATH_INFO=%s\n", getenv("PATH_INFO") ? getenv("PATH_INFO") : "");
        printf("DOCUMENT_ROOT=%s\n", getenv("DOCUMENT_ROOT") ? getenv("DOCUMENT_ROOT") : "");
        printf("RESOLVED=%s\n", path ? path : "");
        free(path);
        return 0;
    }

    headers_reset();

    FILE *body = tmpfile();
    if (!body) {
        fprintf(stderr, "lx_cgi: cannot allocate response buffer\n");
        free(path);
        return 1;
    }
    lx_set_output(body);

    char *src = read_file_all(path);
    if (!src) {
        fprintf(stderr, "lx_cgi: cannot read '%s'\n", path);
        fclose(body);
        lx_set_output(NULL);
        headers_reset();
        free(path);
        return 1;
    }
    char *compiled = compile_template(src);
    free(src);
    if (!compiled) {
        fprintf(stderr, "lx_cgi: invalid template (missing ?>)\n");
        fclose(body);
        lx_set_output(NULL);
        headers_reset();
        free(path);
        return 1;
    }

    int rc = run_script(compiled, path);
    free(compiled);
    free(path);

    lx_set_output(NULL);
    fflush(body);
    headers_send();
    rewind(body);
    char buf[4096];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), body)) > 0) {
        fwrite(buf, 1, nread, stdout);
    }
    fclose(body);
    headers_reset();
    return rc;
}
