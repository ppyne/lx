/**
 * @file lx_cgi.c
 * @brief Minimal CGI wrapper for Lx with <?lx ... ?> blocks.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "natives.h"
#include "array.h"
#include "lx_ext.h"
#include "lx_error.h"
#include "config.h"
#include "blake2.h"
#include "ext_serializer.h"

#if LX_ENABLE_FS
void register_fs_module(void);
#endif
#if LX_ENABLE_JSON
void register_json_module(void);
#endif
#if LX_ENABLE_SERIALIZER
void register_serializer_module(void);
#endif
#if LX_ENABLE_HEX
void register_hex_module(void);
#endif
#if LX_ENABLE_BLAKE2B
void register_blake2b_module(void);
#endif
#if LX_ENABLE_TIME
void register_time_module(void);
#endif
#if LX_ENABLE_ENV
void register_env_module(void);
#endif
#if LX_ENABLE_UTF8
void register_utf8_module(void);
#endif
#if LX_ENABLE_SQLITE
void register_sqlite_module(void);
#endif
#if LX_ENABLE_AEAD
void register_aead_module(void);
#endif
#if LX_ENABLE_ED25519
void register_ed25519_module(void);
#endif
#if LX_ENABLE_EXEC
void register_exec_module(void);
#endif

extern char **environ;

typedef struct {
    char **items;
    int count;
    int cap;
    char *content_type;
} HeaderList;

static HeaderList g_headers = {0};
static int g_headers_sent = 0;

typedef struct {
    char **paths;
    size_t count;
    size_t cap;
} UploadList;

static UploadList g_uploads = {0};

static int append_str(char **buf, size_t *len, size_t *cap, const char *s);
static int append_char(char **buf, size_t *len, size_t *cap, char c);
static Value parse_cookies(const char *hdr);

#if LX_ENABLE_BLAKE2B && LX_ENABLE_SERIALIZER
typedef struct {
    int started;
    int destroyed;
    char *id;
    char *name;
    Value data;
} SessionState;

static SessionState g_session = {0};
#endif

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

static void uploads_reset(void) {
    for (size_t i = 0; i < g_uploads.count; i++) {
        if (g_uploads.paths[i]) {
            unlink(g_uploads.paths[i]);
            free(g_uploads.paths[i]);
        }
    }
    free(g_uploads.paths);
    g_uploads.paths = NULL;
    g_uploads.count = 0;
    g_uploads.cap = 0;
}

static int uploads_track(const char *path) {
    if (!path || !*path) return 0;
    if (g_uploads.count + 1 > g_uploads.cap) {
        size_t cap = g_uploads.cap ? g_uploads.cap * 2 : 8;
        char **paths = (char **)realloc(g_uploads.paths, cap * sizeof(char *));
        if (!paths) return 0;
        g_uploads.paths = paths;
        g_uploads.cap = cap;
    }
    g_uploads.paths[g_uploads.count++] = strdup(path);
    return 1;
}

static int uploads_untrack(const char *path) {
    if (!path || !*path) return 0;
    for (size_t i = 0; i < g_uploads.count; i++) {
        if (g_uploads.paths[i] && strcmp(g_uploads.paths[i], path) == 0) {
            free(g_uploads.paths[i]);
            g_uploads.paths[i] = g_uploads.paths[g_uploads.count - 1];
            g_uploads.paths[g_uploads.count - 1] = NULL;
            g_uploads.count--;
            return 1;
        }
    }
    return 0;
}

static int uploads_is_tracked(const char *path) {
    if (!path || !*path) return 0;
    for (size_t i = 0; i < g_uploads.count; i++) {
        if (g_uploads.paths[i] && strcmp(g_uploads.paths[i], path) == 0) return 1;
    }
    return 0;
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

static Value n_write_blob(Env *env, int argc, Value *argv) {
    (void)env;
    if (argc != 1) return value_undefined();
    if (argv[0].type != VAL_BLOB || !argv[0].blob) return value_undefined();
    Blob *b = argv[0].blob;
    FILE *out = lx_get_output();
    if (!out) return value_undefined();
    size_t n = 0;
    if (b->data && b->len > 0) {
        n = fwrite(b->data, 1, b->len, out);
    }
    return value_int((lx_int_t)n);
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return 0;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return 0; }
    char buf[8192];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { fclose(in); fclose(out); return 0; }
    }
    fclose(in);
    fclose(out);
    return 1;
}

static Value n_move_uploaded_file(Env *env, int argc, Value *argv) {
    (void)env;
    if (argc != 2) return value_bool(0);
    Value srcv = value_to_string(argv[0]);
    Value dstv = value_to_string(argv[1]);
    const char *src = srcv.s ? srcv.s : "";
    const char *dst = dstv.s ? dstv.s : "";
    if (!uploads_is_tracked(src) || !*dst) {
        value_free(srcv);
        value_free(dstv);
        return value_bool(0);
    }
    int ok = (rename(src, dst) == 0);
    if (!ok) {
        ok = copy_file(src, dst);
        if (ok) unlink(src);
    }
    if (ok) uploads_untrack(src);
    value_free(srcv);
    value_free(dstv);
    return value_bool(ok);
}

static int format_http_date(lx_int_t ts, char *out, size_t out_len) {
    if (!out || out_len == 0) return 0;
    if (ts <= 0) return 0;
    time_t t = (time_t)ts;
    struct tm *gt = gmtime(&t);
    if (!gt) return 0;
    return strftime(out, out_len, "%a, %d %b %Y %H:%M:%S GMT", gt) > 0;
}

static Value n_setcookie(Env *env, int argc, Value *argv) {
    (void)env;
    if (argc < 2) return value_bool(0);
    Value namev = value_to_string(argv[0]);
    Value valv = value_to_string(argv[1]);
    const char *name = namev.s ? namev.s : "";
    const char *val = valv.s ? valv.s : "";
    if (!*name) {
        value_free(namev);
        value_free(valv);
        return value_bool(0);
    }

    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    append_str(&buf, &len, &cap, "Set-Cookie: ");
    append_str(&buf, &len, &cap, name);
    append_char(&buf, &len, &cap, '=');
    append_str(&buf, &len, &cap, val);

    if (argc >= 3 && argv[2].type != VAL_NULL && argv[2].type != VAL_UNDEFINED) {
        lx_int_t exp = value_to_int(argv[2]).i;
        char date[64];
        if (format_http_date(exp, date, sizeof(date))) {
            append_str(&buf, &len, &cap, "; Expires=");
            append_str(&buf, &len, &cap, date);
        }
    }

    if (argc >= 4 && argv[3].type != VAL_NULL && argv[3].type != VAL_UNDEFINED) {
        Value pv = value_to_string(argv[3]);
        if (pv.s && *pv.s) {
            append_str(&buf, &len, &cap, "; Path=");
            append_str(&buf, &len, &cap, pv.s);
        }
        value_free(pv);
    }

    if (argc >= 5 && argv[4].type != VAL_NULL && argv[4].type != VAL_UNDEFINED) {
        Value dv = value_to_string(argv[4]);
        if (dv.s && *dv.s) {
            append_str(&buf, &len, &cap, "; Domain=");
            append_str(&buf, &len, &cap, dv.s);
        }
        value_free(dv);
    }

    if (argc >= 6 && value_is_true(argv[5])) {
        append_str(&buf, &len, &cap, "; Secure");
    }

    if (argc >= 7 && value_is_true(argv[6])) {
        append_str(&buf, &len, &cap, "; HttpOnly");
    }

    headers_add(buf ? buf : "");
    free(buf);
    value_free(namev);
    value_free(valv);
    return value_bool(1);
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

#if LX_ENABLE_BLAKE2B && LX_ENABLE_SERIALIZER
static int read_random(uint8_t *out, size_t len) {
    if (!out || len == 0) return 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return 0;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, out + off, len - off);
        if (n <= 0) { close(fd); return 0; }
        off += (size_t)n;
    }
    close(fd);
    return 1;
}

static char *base64url_encode(const uint8_t *data, size_t len) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t out_len = (len / 3) * 4 + ((len % 3) ? (len % 3) + 1 : 0);
    char *out = (char *)malloc(out_len + 1);
    if (!out) return NULL;
    size_t i = 0;
    size_t j = 0;
    while (i + 3 <= len) {
        uint32_t v = (uint32_t)data[i] << 16 | (uint32_t)data[i + 1] << 8 | data[i + 2];
        out[j++] = table[(v >> 18) & 0x3F];
        out[j++] = table[(v >> 12) & 0x3F];
        out[j++] = table[(v >> 6) & 0x3F];
        out[j++] = table[v & 0x3F];
        i += 3;
    }
    if (i < len) {
        uint32_t v = (uint32_t)data[i] << 16;
        out[j++] = table[(v >> 18) & 0x3F];
        if (i + 1 < len) {
            v |= (uint32_t)data[i + 1] << 8;
            out[j++] = table[(v >> 12) & 0x3F];
            out[j++] = table[(v >> 6) & 0x3F];
        } else {
            out[j++] = table[(v >> 12) & 0x3F];
        }
    }
    out[j] = '\0';
    return out;
}

static void blake2b_hash(const uint8_t *data, size_t len, uint8_t out[32]) {
    blake2b_state st;
    blake2b_init(&st, 32);
    blake2b_update(&st, data, len);
    blake2b_final(&st, out, 32);
}

static char *hex_encode(const uint8_t *data, size_t len) {
    static const char *hex = "0123456789abcdef";
    char *out = (char *)malloc(len * 2 + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[data[i] >> 4];
        out[i * 2 + 1] = hex[data[i] & 0x0F];
    }
    out[len * 2] = '\0';
    return out;
}

static char *session_file_path(const char *id) {
    if (!id || !*id) return NULL;
    uint8_t hash[32];
    blake2b_hash((const uint8_t *)id, strlen(id), hash);
    char *hex = hex_encode(hash, sizeof(hash));
    if (!hex) return NULL;
    const char *dir = SESSION_FILE_PATH;
    if (!dir || !*dir) dir = "/tmp";
    size_t dlen = strlen(dir);
    size_t hlen = strlen(hex);
    const char *prefix = "lxsession_";
    size_t plen = strlen(prefix);
    char *out = (char *)malloc(dlen + 1 + plen + hlen + 1);
    if (!out) { free(hex); return NULL; }
    memcpy(out, dir, dlen);
    out[dlen] = '/';
    memcpy(out + dlen + 1, prefix, plen);
    memcpy(out + dlen + 1 + plen, hex, hlen + 1);
    free(hex);
    return out;
}

static int session_gc_should_run(void) {
    uint32_t v = 0;
    if (!read_random((uint8_t *)&v, sizeof(v))) return 0;
    if (SESSION_GC_DIV <= 0) return 0;
    return (int)(v % SESSION_GC_DIV) < SESSION_GC_PROB;
}

static void session_gc(void) {
    const char *dir = SESSION_FILE_PATH;
    if (!dir || !*dir) dir = "/tmp";
    DIR *d = opendir(dir);
    if (!d) return;
    time_t now = time(NULL);
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "lxsession_", 10) != 0) continue;
        size_t dlen = strlen(dir);
        size_t nlen = strlen(ent->d_name);
        char *path = (char *)malloc(dlen + 1 + nlen + 1);
        if (!path) continue;
        memcpy(path, dir, dlen);
        path[dlen] = '/';
        memcpy(path + dlen + 1, ent->d_name, nlen + 1);
        struct stat st;
        if (stat(path, &st) == 0) {
            if (now - st.st_mtime > SESSION_TTL) {
                unlink(path);
            }
        }
        free(path);
    }
    closedir(d);
}

static int session_load(const char *id, Value *out) {
    if (!out) return 0;
    char *path = session_file_path(id);
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) == 0) {
        time_t now = time(NULL);
        if (now - st.st_mtime > SESSION_TTL) {
            unlink(path);
            free(path);
            return 0;
        }
    }
    FILE *f = fopen(path, "rb");
    if (!f) { free(path); return 0; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); free(path); return 0; }
    long size = ftell(f);
    if (size < 0) { fclose(f); free(path); return 0; }
    rewind(f);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); free(path); return 0; }
    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[n] = '\0';
    int ok = 0;
    Value v = lx_unserialize_string(buf, &ok);
    free(buf);
    free(path);
    if (!ok || v.type != VAL_ARRAY || !v.a) {
        value_free(v);
        return 0;
    }
    *out = v;
    return 1;
}

static int session_save(const char *id, Value data) {
    char *path = session_file_path(id);
    if (!path) return 0;
    char *payload = NULL;
    size_t payload_len = 0;
    if (!lx_serialize(data, &payload, &payload_len)) { free(path); return 0; }
    const char *dir = SESSION_FILE_PATH;
    if (!dir || !*dir) dir = "/tmp";
    char tmp_path[PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s/lxsession_tmp_XXXXXX", dir);
    int fd = mkstemp(tmp_path);
    if (fd < 0) { free(path); free(payload); return 0; }
    FILE *f = fdopen(fd, "wb");
    if (!f) { close(fd); unlink(tmp_path); free(path); free(payload); return 0; }
    size_t n = fwrite(payload, 1, payload_len, f);
    fclose(f);
    free(payload);
    if (n != payload_len) { unlink(tmp_path); free(path); return 0; }
    chmod(tmp_path, SESSION_FILE_PERMISSIONS);
    int rc = rename(tmp_path, path);
    free(path);
    if (rc != 0) { unlink(tmp_path); return 0; }
    return 1;
}

static char *session_generate_id(void) {
    uint8_t seed[32];
    if (!read_random(seed, sizeof(seed))) return NULL;
    uint8_t hash[32];
    blake2b_hash(seed, sizeof(seed), hash);
    return base64url_encode(hash, sizeof(hash));
}

static const char *session_name_value(void) {
    if (g_session.name) return g_session.name;
    return SESSION_NAME;
}

static void session_set_cookie(const char *id, time_t expires) {
    const char *name = session_name_value();
    if (!name || !*name || !id) return;
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;
    append_str(&buf, &len, &cap, "Set-Cookie: ");
    append_str(&buf, &len, &cap, name);
    append_char(&buf, &len, &cap, '=');
    append_str(&buf, &len, &cap, id);
    append_str(&buf, &len, &cap, "; Path=/");
    append_str(&buf, &len, &cap, "; HttpOnly");
    if (expires > 0) {
        char date[64];
        if (format_http_date((lx_int_t)expires, date, sizeof(date))) {
            append_str(&buf, &len, &cap, "; Expires=");
            append_str(&buf, &len, &cap, date);
        }
    }
    headers_add(buf ? buf : "");
    free(buf);
}

static Value n_session_name(Env *env, int argc, Value *argv) {
    (void)env;
    const char *cur = session_name_value();
    if (argc >= 1) {
        Value nv = value_to_string(argv[0]);
        if (nv.s && *nv.s) {
            free(g_session.name);
            g_session.name = strdup(nv.s);
        }
        value_free(nv);
    }
    return value_string(cur);
}

static Value n_session_id(Env *env, int argc, Value *argv) {
    (void)env;
    const char *cur = g_session.id ? g_session.id : "";
    if (argc >= 1) {
        Value iv = value_to_string(argv[0]);
        if (iv.s && *iv.s) {
            free(g_session.id);
            g_session.id = strdup(iv.s);
        }
        value_free(iv);
    }
    return value_string(cur);
}

static Value n_session_start(Env *env, int argc, Value *argv) {
    if (g_session.started) return value_bool(1);
    if (argc >= 1 && argv[0].type != VAL_UNDEFINED && argv[0].type != VAL_NULL) {
        Value nv = value_to_string(argv[0]);
        if (nv.s && *nv.s) {
            free(g_session.name);
            g_session.name = strdup(nv.s);
        }
        value_free(nv);
    }
    if (session_gc_should_run()) session_gc();

    Value cookies = parse_cookies(getenv("HTTP_COOKIE"));
    const char *name = session_name_value();
    char *id = NULL;
    if (cookies.a && name) {
        Value cv = array_get(cookies.a, key_string(name));
        if (cv.type == VAL_STRING && cv.s && *cv.s) {
            id = strdup(cv.s);
        }
        value_free(cv);
    }
    value_free(cookies);

    if (!id) id = session_generate_id();
    if (!id) return value_bool(0);

    Value data = value_array();
    if (session_load(id, &data) == 0) {
        value_free(data);
        data = value_array();
    }

    g_session.started = 1;
    g_session.destroyed = 0;
    free(g_session.id);
    g_session.id = id;
    value_free(g_session.data);
    g_session.data = data;
    env_set(env, "_SESSION", value_copy(data));
    return value_bool(1);
}

static Value n_session_destroy(Env *env, int argc, Value *argv) {
    (void)argc;
    (void)argv;
    if (!g_session.started) return value_bool(0);
    if (g_session.id) {
        char *path = session_file_path(g_session.id);
        if (path) {
            unlink(path);
            free(path);
        }
    }
    g_session.destroyed = 1;
    g_session.started = 0;
    if (env) env_set(env, "_SESSION", value_array());
    session_set_cookie("", 1);
    return value_bool(1);
}

static Value n_session_regenerate_id(Env *env, int argc, Value *argv) {
    (void)env;
    if (!g_session.started) return value_bool(0);
    int del = 0;
    if (argc >= 1) del = value_is_true(argv[0]);
    char *old_id = g_session.id ? strdup(g_session.id) : NULL;
    char *new_id = session_generate_id();
    if (!new_id) { free(old_id); return value_bool(0); }
    free(g_session.id);
    g_session.id = new_id;
    if (del && old_id) {
        char *path = session_file_path(old_id);
        if (path) {
            unlink(path);
            free(path);
        }
    }
    free(old_id);
    return value_bool(1);
}

static void session_reset(void) {
    g_session.started = 0;
    g_session.destroyed = 0;
    free(g_session.id);
    free(g_session.name);
    g_session.id = NULL;
    g_session.name = NULL;
    value_free(g_session.data);
    g_session.data = value_undefined();
}
#endif
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

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static char *dup_trim_range(const char *s, size_t n) {
    size_t start = 0;
    size_t end = n;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) start++;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) end--;
    return dup_range(s + start, end - start);
}

static const char *memmem_local(const char *hay, size_t hlen, const char *needle, size_t nlen) {
    if (!needle || nlen == 0 || !hay || hlen < nlen) return NULL;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (hay[i] == needle[0] && memcmp(hay + i, needle, nlen) == 0) return hay + i;
    }
    return NULL;
}

static char *disposition_param(const char *line, const char *key) {
    size_t klen = strlen(key);
    const char *p = line;
    while ((p = strstr(p, key)) != NULL) {
        if (p != line && p[-1] != ';' && p[-1] != ' ') { p += klen; continue; }
        p += klen;
        if (*p != '=') continue;
        p++;
        if (*p == '"') {
            p++;
            const char *end = strchr(p, '"');
            if (!end) return NULL;
            return dup_range(p, (size_t)(end - p));
        }
        const char *end = p;
        while (*end && *end != ';' && *end != '\r' && *end != '\n') end++;
        return dup_range(p, (size_t)(end - p));
    }
    return NULL;
}

typedef struct {
    char *name;
    char *filename;
    char *content_type;
} PartInfo;

static void part_info_reset(PartInfo *info) {
    if (!info) return;
    free(info->name);
    free(info->filename);
    free(info->content_type);
    info->name = NULL;
    info->filename = NULL;
    info->content_type = NULL;
}

static void parse_part_headers(const char *hdr, size_t len, PartInfo *info) {
    const char *p = hdr;
    const char *end = hdr + len;
    while (p < end) {
        const char *eol = memmem_local(p, (size_t)(end - p), "\r\n", 2);
        size_t line_len = eol ? (size_t)(eol - p) : (size_t)(end - p);
        if (line_len == 0) break;
        if (line_len >= 20 && strncasecmp(p, "Content-Disposition:", 20) == 0) {
            const char *val = p + 20;
            while (*val == ' ' || *val == '\t') val++;
            char *name = disposition_param(val, "name");
            char *filename = disposition_param(val, "filename");
            if (name) { free(info->name); info->name = name; }
            if (filename) { free(info->filename); info->filename = filename; }
        } else if (line_len >= 13 && strncasecmp(p, "Content-Type:", 13) == 0) {
            const char *val = p + 13;
            while (*val == ' ' || *val == '\t') val++;
            char *ctype = dup_range(val, line_len - (size_t)(val - p));
            if (ctype) {
                free(info->content_type);
                info->content_type = ctype;
            }
        }
        if (!eol) break;
        p = eol + 2;
    }
}

static void add_post_value(Value post, const char *name, Value v) {
    if (!post.a || !name) { value_free(v); return; }
    size_t nlen = strlen(name);
    if (nlen >= 2 && name[nlen - 2] == '[' && name[nlen - 1] == ']') {
        char *base = dup_range(name, nlen - 2);
        if (!base) { value_free(v); return; }
        Value *slot = array_get_ref(post.a, key_string(base));
        if (slot->type != VAL_ARRAY) {
            Value old = *slot;
            *slot = value_array();
            if (old.type != VAL_UNDEFINED) {
                array_set(slot->a, key_int(0), old);
            } else {
                value_free(old);
            }
        }
        lx_int_t idx = array_next_index(slot->a);
        array_set(slot->a, key_int(idx), v);
        free(base);
        return;
    }
    array_set(post.a, key_string(name), v);
}

static void file_entry_append(Value entry, const char *key, Value v) {
    Value *slot = array_get_ref(entry.a, key_string(key));
    if (slot->type != VAL_ARRAY) {
        Value old = *slot;
        *slot = value_array();
        if (old.type != VAL_UNDEFINED) {
            array_set(slot->a, key_int(0), old);
        } else {
            value_free(old);
        }
    }
    lx_int_t idx = array_next_index(slot->a);
    array_set(slot->a, key_int(idx), v);
}

static void add_file_entry(Value files, const char *field, const char *name,
                           const char *type, const char *tmp_name,
                           size_t size, int error) {
    if (!files.a || !field) return;
    Value *slot = array_get_ref(files.a, key_string(field));
    if (slot->type == VAL_UNDEFINED) {
        Value entry = value_array();
        array_set(entry.a, key_string("name"), value_string(name ? name : ""));
        array_set(entry.a, key_string("type"), value_string(type ? type : ""));
        array_set(entry.a, key_string("tmp_name"), value_string(tmp_name ? tmp_name : ""));
        array_set(entry.a, key_string("size"), value_int((lx_int_t)size));
        array_set(entry.a, key_string("error"), value_int(error));
        *slot = entry;
        return;
    }
    if (slot->type != VAL_ARRAY || !slot->a) {
        value_free(*slot);
        *slot = value_array();
    }
    file_entry_append(*slot, "name", value_string(name ? name : ""));
    file_entry_append(*slot, "type", value_string(type ? type : ""));
    file_entry_append(*slot, "tmp_name", value_string(tmp_name ? tmp_name : ""));
    file_entry_append(*slot, "size", value_int((lx_int_t)size));
    file_entry_append(*slot, "error", value_int(error));
}

static int read_body(size_t len, char **out) {
    if (!out) return 0;
    *out = NULL;
    if (len == 0) return 1;
    char *buf = (char *)malloc(len + 1);
    if (!buf) return 0;
    size_t n = fread(buf, 1, len, stdin);
    buf[n] = '\0';
    *out = buf;
    return n == len;
}

static void discard_body(size_t len) {
    char buf[4096];
    while (len > 0) {
        size_t chunk = len > sizeof(buf) ? sizeof(buf) : len;
        size_t n = fread(buf, 1, chunk, stdin);
        if (n == 0) break;
        len -= n;
    }
}

static int parse_content_length(size_t *out_len) {
    const char *len_s = getenv("CONTENT_LENGTH");
    if (!len_s || !*len_s) return 0;
    char *end = NULL;
    unsigned long long v = strtoull(len_s, &end, 10);
    if (!end || end == len_s) return 0;
    if (v > SIZE_MAX) return 0;
    *out_len = (size_t)v;
    return 1;
}

static int extract_boundary(const char *ctype, char **out) {
    if (!ctype || !out) return 0;
    const char *b = strstr(ctype, "boundary=");
    if (!b) return 0;
    b += 9;
    if (*b == '"') {
        b++;
        const char *end = strchr(b, '"');
        if (!end) return 0;
        *out = dup_range(b, (size_t)(end - b));
        return *out != NULL;
    }
    const char *end = b;
    while (*end && *end != ';' && *end != ' ' && *end != '\t') end++;
    *out = dup_range(b, (size_t)(end - b));
    return *out != NULL;
}

static void parse_multipart_body(const char *data, size_t len, const char *boundary,
                                 Value post, Value files) {
    if (!data || !boundary || !*boundary) return;
    size_t b_len = strlen(boundary);
    size_t marker_len = b_len + 2;
    char *marker = (char *)malloc(marker_len + 1);
    if (!marker) return;
    marker[0] = '-';
    marker[1] = '-';
    memcpy(marker + 2, boundary, b_len);
    marker[marker_len] = '\0';

    const char *p = data;
    const char *end = data + len;
    const char *pos = memmem_local(p, len, marker, marker_len);
    if (!pos) { free(marker); return; }
    p = pos + marker_len;
    if (p + 2 <= end && p[0] == '-' && p[1] == '-') { free(marker); return; }
    if (p + 2 <= end && p[0] == '\r' && p[1] == '\n') p += 2;

    size_t upload_count = 0;

    while (p < end) {
        const char *hdr_end = memmem_local(p, (size_t)(end - p), "\r\n\r\n", 4);
        if (!hdr_end) break;
        PartInfo info = {0};
        parse_part_headers(p, (size_t)(hdr_end - p), &info);
        const char *content_start = hdr_end + 4;

        const char *search = content_start;
        const char *next = NULL;
        while (search < end) {
            next = memmem_local(search, (size_t)(end - search), marker, marker_len);
            if (!next) break;
            if (next >= data + 2 && next[-2] == '\r' && next[-1] == '\n') break;
            search = next + 1;
        }
        if (!next) { part_info_reset(&info); break; }
        const char *content_end = next - 2;
        if (content_end < content_start) content_end = content_start;
        size_t content_len = (size_t)(content_end - content_start);

        if (info.name && *info.name) {
            if (info.filename) {
                int err = 0;
                char tmp_path[PATH_MAX];
                tmp_path[0] = '\0';
                if (!FILE_UPLOADS) {
                    err = 1;
                } else if (upload_count >= (size_t)MAX_FILE_UPLOADS) {
                    err = 1;
                } else if (content_len > (size_t)UPLOAD_MAX_FILESIZE) {
                    err = 1;
                } else if (info.filename[0] == '\0') {
                    err = 4;
                } else {
                    const char *dir = UPLOAD_TMP_DIR;
                    if (!dir || !*dir) dir = "/tmp";
                    snprintf(tmp_path, sizeof(tmp_path), "%s/lx_upload_XXXXXX", dir);
                    int fd = mkstemp(tmp_path);
                    if (fd == -1) {
                        err = 6;
                    } else {
                        FILE *out = fdopen(fd, "wb");
                        if (!out) {
                            close(fd);
                            unlink(tmp_path);
                            tmp_path[0] = '\0';
                            err = 7;
                        } else {
                            size_t written = fwrite(content_start, 1, content_len, out);
                            fclose(out);
                            if (written != content_len) {
                                unlink(tmp_path);
                                tmp_path[0] = '\0';
                                err = 7;
                            } else {
                                uploads_track(tmp_path);
                                upload_count++;
                            }
                        }
                    }
                }
                add_file_entry(files, info.name, info.filename,
                               info.content_type ? info.content_type : "",
                               tmp_path[0] ? tmp_path : "",
                               content_len, err);
            } else {
                Value v = value_string_n(content_start, content_len);
                add_post_value(post, info.name, v);
            }
        }

        part_info_reset(&info);
        p = next + marker_len;
        if (p + 2 <= end && p[0] == '-' && p[1] == '-') break;
        if (p + 2 <= end && p[0] == '\r' && p[1] == '\n') p += 2;
    }

    free(marker);
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

static Value parse_cookies(const char *hdr) {
    Value out = value_array();
    if (!hdr) return out;
    const char *p = hdr;
    while (*p) {
        while (*p == ';' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        const char *end = strchr(p, ';');
        if (!end) end = p + strlen(p);
        const char *eq = memchr(p, '=', (size_t)(end - p));
        if (!eq) {
            p = end;
            continue;
        }
        char *kraw = dup_trim_range(p, (size_t)(eq - p));
        char *vraw = dup_trim_range(eq + 1, (size_t)(end - eq - 1));
        if (!kraw || !vraw) { free(kraw); free(vraw); return out; }
        char *k = url_decode(kraw);
        char *v = url_decode(vraw);
        free(kraw);
        free(vraw);
        if (!k || !v) { free(k); free(v); return out; }
        array_set(out.a, key_string(k), value_string(v));
        free(k);
        free(v);
        p = end;
    }
    return out;
}

static void env_set_array(Env *env, const char *name, Value arr) {
    env_set(env, name, arr);
}

static Value merge_request(Value get, Value post) {
    Value out = value_array();
    if (get.type == VAL_ARRAY && get.a) {
        for (size_t i = 0; i < get.a->size; i++) {
            Key key = get.a->entries[i].key;
            Value v = value_copy(get.a->entries[i].value);
            if (key.type == KEY_STRING) array_set(out.a, key_string(key.s), v);
            else array_set(out.a, key_int(key.i), v);
        }
    }
    if (post.type == VAL_ARRAY && post.a) {
        for (size_t i = 0; i < post.a->size; i++) {
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
    Value files = value_array();
    const char *method = getenv("REQUEST_METHOD");
    const char *ctype = getenv("CONTENT_TYPE");
    if (method && strcmp(method, "POST") == 0 && ctype) {
        size_t content_len = 0;
        if (parse_content_length(&content_len)) {
            if (content_len > (size_t)POST_MAX_SIZE) {
                discard_body(content_len);
            } else if (strstr(ctype, "multipart/form-data") == ctype) {
                char *boundary = NULL;
                if (extract_boundary(ctype, &boundary)) {
                    char *body = NULL;
                    if (read_body(content_len, &body) && body) {
                        parse_multipart_body(body, content_len, boundary, post, files);
                    }
                    free(body);
                }
                free(boundary);
            } else if (strstr(ctype, "application/x-www-form-urlencoded") == ctype) {
                char *body = NULL;
                if (read_body(content_len, &body) && body) {
                    Value parsed = parse_kv(body);
                    value_free(post);
                    post = parsed;
                }
                free(body);
            } else {
                discard_body(content_len);
            }
        }
    }
    Value req = merge_request(get, post);
    Value server = build_server_env();
    Value cookies = parse_cookies(getenv("HTTP_COOKIE"));

    env_set_array(global, "_GET", get);
    env_set_array(global, "_POST", post);
    env_set_array(global, "_REQUEST", req);
    env_set_array(global, "_SERVER", server);
    env_set_array(global, "_FILES", files);
    env_set_array(global, "_COOKIE", cookies);
}

static int run_script(const char *source, const char *filename) {
    Parser parser;
    lexer_init(&parser.lexer, source, filename);
    parser.current.type = TOK_ERROR;
    parser.previous.type = TOK_ERROR;
    lx_error_clear();

    AstNode *program = parse_program(&parser);
    if (lx_has_error() || !program) {
        FILE *out = LX_CGI_DISPLAY_ERRORS ? lx_get_output() : stderr;
        lx_print_error(out);
        return 1;
    }

    Env *global = env_new(NULL);
    install_stdlib();
    register_function("header", n_header);
    register_function("write_blob", n_write_blob);
    register_function("move_uploaded_file", n_move_uploaded_file);
    register_function("setcookie", n_setcookie);
#if LX_ENABLE_BLAKE2B && LX_ENABLE_SERIALIZER
    register_function("session_start", n_session_start);
    register_function("session_destroy", n_session_destroy);
    register_function("session_regenerate_id", n_session_regenerate_id);
    register_function("session_id", n_session_id);
    register_function("session_name", n_session_name);
#endif
#if LX_ENABLE_FS
    register_fs_module();
#endif
#if LX_ENABLE_JSON
    register_json_module();
#endif
#if LX_ENABLE_SERIALIZER
    register_serializer_module();
#endif
#if LX_ENABLE_HEX
    register_hex_module();
#endif
#if LX_ENABLE_BLAKE2B
    register_blake2b_module();
#endif
#if LX_ENABLE_TIME
    register_time_module();
#endif
#if LX_ENABLE_ENV
    register_env_module();
#endif
#if LX_ENABLE_UTF8
    register_utf8_module();
#endif
#if LX_ENABLE_SQLITE
    register_sqlite_module();
#endif
#if LX_ENABLE_AEAD
    register_aead_module();
#endif
#if LX_ENABLE_ED25519
    register_ed25519_module();
#endif
#if LX_ENABLE_EXEC
    register_exec_module();
#endif
    lx_init_modules(global);
    install_std_env(global);

    EvalResult r = eval_program(program, global);
    if (lx_has_error()) {
        FILE *out = LX_CGI_DISPLAY_ERRORS ? lx_get_output() : stderr;
        lx_print_error(out);
        value_free(r.value);
        env_free(global);
        return 1;
    }
#if LX_ENABLE_BLAKE2B && LX_ENABLE_SERIALIZER
    if (g_session.started && !g_session.destroyed && g_session.id) {
        Value sess = env_get(global, "_SESSION");
        if (sess.type == VAL_ARRAY) {
            value_free(g_session.data);
            g_session.data = value_copy(sess);
        }
        value_free(sess);
        session_save(g_session.id, g_session.data);
        if (SESSION_TTL > 0) {
            time_t exp = time(NULL) + SESSION_TTL;
            session_set_cookie(g_session.id, exp);
        } else {
            session_set_cookie(g_session.id, 0);
        }
    }
#endif
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
    uploads_reset();
#if LX_ENABLE_BLAKE2B && LX_ENABLE_SERIALIZER
    session_reset();
#endif
    return rc;
}
