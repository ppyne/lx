/**
 * @file ext_fs.c
 * @brief Filesystem extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static const char *fs_temp_dir(void) {
    const char *vars[] = { "TMPDIR", "TMP", "TEMP", "TEMPDIR" };
    for (size_t i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
        const char *v = getenv(vars[i]);
        if (v && *v) return v;
    }
    return "/tmp";
}

static Value n_sys_get_temp_dir(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_string("");
    return value_string(fs_temp_dir());
}

static Value n_tempnam(Env *env, int argc, Value *argv){
    (void)env;
    const char *prefix = "lx";
    if (argc == 1 && argv[0].type == VAL_STRING && argv[0].s && *argv[0].s) {
        prefix = argv[0].s;
    } else if (argc != 0 && argc != 1) {
        return value_undefined();
    }

    const char *dir = fs_temp_dir();
    size_t dlen = strlen(dir);
    size_t plen = strlen(prefix);
    if (dlen + 1 + plen + 6 + 1 >= PATH_MAX) return value_undefined();

    char path[PATH_MAX];
    int add_slash = (dlen > 0 && dir[dlen - 1] != '/');
    snprintf(path, sizeof(path), "%s%s%sXXXXXX", dir, add_slash ? "/" : "", prefix);

    int fd = mkstemp(path);
    if (fd < 0) return value_undefined();
    close(fd);
    return value_string(path);
}

static Value n_file_get_contents(Env *env, int argc, Value *argv){
    (void)env;
    int want_blob = 0;
    if (argc == 2 && argv[1].type == VAL_BOOL) {
        want_blob = argv[1].b;
    }
    if ((argc != 1 && argc != 2) || argv[0].type != VAL_STRING) return value_undefined();
    const char *path = argv[0].s ? argv[0].s : "";
    FILE *f = fopen(path, "rb");
    if (!f) return value_undefined();
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return value_undefined(); }
    long size = ftell(f);
    if (size < 0) { fclose(f); return value_undefined(); }
    rewind(f);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return value_undefined(); }
    size_t readn = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[readn] = '\0';
    if (want_blob) {
        Value out = value_blob_n((const unsigned char *)buf, readn);
        free(buf);
        return out;
    }
    size_t out_len = readn;
    char *nul = memchr(buf, 0, readn);
    if (nul) out_len = (size_t)(nul - buf);
    Value out = value_string_n(buf, out_len);
    free(buf);
    return out;
}

static Value n_file_put_contents(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_int(0);
    const char *path = argv[0].s ? argv[0].s : "";
    FILE *f = fopen(path, "wb");
    if (!f) return value_int(0);
    size_t n = 0;
    if (argv[1].type == VAL_BLOB && argv[1].blob) {
        n = fwrite(argv[1].blob->data, 1, argv[1].blob->len, f);
    } else {
        Value sv = value_to_string(argv[1]);
        const char *s = sv.s ? sv.s : "";
        n = fwrite(s, 1, strlen(s), f);
        value_free(sv);
    }
    fclose(f);
    return value_int((int)n);
}

static Value n_file_exists(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    struct stat st;
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(stat(path, &st) == 0);
}

static Value n_file_size(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    struct stat st;
    const char *path = argv[0].s ? argv[0].s : "";
    if (stat(path, &st) != 0) return value_undefined();
    return value_int((int)st.st_size);
}

static Value n_is_dir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    struct stat st;
    const char *path = argv[0].s ? argv[0].s : "";
    if (stat(path, &st) != 0) return value_bool(0);
    return value_bool(S_ISDIR(st.st_mode));
}

static Value n_is_file(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    struct stat st;
    const char *path = argv[0].s ? argv[0].s : "";
    if (stat(path, &st) != 0) return value_bool(0);
    return value_bool(S_ISREG(st.st_mode));
}

static Value n_mkdir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(mkdir(path, 0755) == 0);
}

static Value n_rmdir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(rmdir(path) == 0);
}

static Value n_unlink(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(unlink(path) == 0);
}

static Value n_copy(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const char *src = argv[0].s ? argv[0].s : "";
    const char *dst = argv[1].s ? argv[1].s : "";
    FILE *in = fopen(src, "rb");
    if (!in) return value_bool(0);
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return value_bool(0);
    }
    char buf[8192];
    size_t n = 0;
    int ok = 1;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = 0;
            break;
        }
    }
    if (ferror(in)) ok = 0;
    fclose(in);
    fclose(out);
    if (!ok) unlink(dst);
    return value_bool(ok);
}

static Value n_rename(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const char *src = argv[0].s ? argv[0].s : "";
    const char *dst = argv[1].s ? argv[1].s : "";
    if (strchr(dst, '/') == NULL) {
        const char *slash = strrchr(src, '/');
        if (slash && slash != src) {
            size_t dirlen = (size_t)(slash - src);
            size_t dstlen = strlen(dst);
            char *buf = (char *)malloc(dirlen + 1 + dstlen + 1);
            if (!buf) return value_bool(0);
            memcpy(buf, src, dirlen);
            buf[dirlen] = '/';
            memcpy(buf + dirlen + 1, dst, dstlen);
            buf[dirlen + 1 + dstlen] = '\0';
            int ok = (rename(src, buf) == 0);
            free(buf);
            return value_bool(ok);
        }
        if (slash && slash == src) {
            size_t dstlen = strlen(dst);
            char *buf = (char *)malloc(1 + dstlen + 1);
            if (!buf) return value_bool(0);
            buf[0] = '/';
            memcpy(buf + 1, dst, dstlen);
            buf[1 + dstlen] = '\0';
            int ok = (rename(src, buf) == 0);
            free(buf);
            return value_bool(ok);
        }
    }
    return value_bool(rename(src, dst) == 0);
}

static Value n_chmod(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    int mode = (int)value_to_int(argv[1]).i;
    return value_bool(chmod(path, (mode_t)mode) == 0);
}

static Value n_pwd(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_string("");
    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof(buf))) return value_string("");
    return value_string(buf);
}

static Value n_pathinfo(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc != 1 || argv[0].type != VAL_STRING) return out;
    const char *path = argv[0].s ? argv[0].s : "";

    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    char *dirname = NULL;
    if (slash) {
        size_t dlen = (size_t)(slash - path);
        if (dlen == 0) dirname = strdup("/");
        else dirname = dup_range(path, dlen);
    } else {
        dirname = strdup(".");
    }

    const char *dot = strrchr(base, '.');
    char *extension = NULL;
    char *filename = NULL;
    if (dot && dot != base) {
        extension = strdup(dot + 1);
        filename = dup_range(base, (size_t)(dot - base));
    } else {
        extension = strdup("");
        filename = strdup(base);
    }

    array_set(out.a, key_string("dirname"), value_string(dirname ? dirname : ""));
    array_set(out.a, key_string("basename"), value_string(base));
    array_set(out.a, key_string("extension"), value_string(extension ? extension : ""));
    array_set(out.a, key_string("filename"), value_string(filename ? filename : ""));

    free(dirname);
    free(extension);
    free(filename);
    return out;
}

static int name_cmp(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

static Value n_list_dir(Env *env, int argc, Value *argv){
    (void)env;
    Value out = value_array();
    if (argc != 1 || argv[0].type != VAL_STRING) return out;
    const char *path = argv[0].s ? argv[0].s : "";
    DIR *dir = opendir(path);
    if (!dir) return out;

    char **names = NULL;
    size_t count = 0;
    size_t cap = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (count == cap) {
            size_t ncap = cap ? cap * 2 : 8;
            char **nn = (char **)realloc(names, ncap * sizeof(char *));
            if (!nn) break;
            names = nn;
            cap = ncap;
        }
        names[count++] = strdup(ent->d_name);
    }
    closedir(dir);

    qsort(names, count, sizeof(char *), name_cmp);
    for (size_t i = 0; i < count; i++) {
        array_set(out.a, key_int((int)i), value_string(names[i] ? names[i] : ""));
        free(names[i]);
    }
    free(names);
    return out;
}

static void fs_module_init(Env *global){
    lx_register_function("file_get_contents", n_file_get_contents);
    lx_register_function("file_put_contents", n_file_put_contents);
    lx_register_function("file_exists", n_file_exists);
    lx_register_function("file_size", n_file_size);
    lx_register_function("is_dir", n_is_dir);
    lx_register_function("is_file", n_is_file);
    lx_register_function("mkdir", n_mkdir);
    lx_register_function("rmdir", n_rmdir);
    lx_register_function("unlink", n_unlink);
    lx_register_function("copy", n_copy);
    lx_register_function("cp", n_copy);
    lx_register_function("rename", n_rename);
    lx_register_function("mv", n_rename);
    lx_register_function("chmod", n_chmod);
    lx_register_function("pwd", n_pwd);
    lx_register_function("sys_get_temp_dir", n_sys_get_temp_dir);
    lx_register_function("tempnam", n_tempnam);
    lx_register_function("pathinfo", n_pathinfo);
    lx_register_function("list_dir", n_list_dir);
    (void)global;
}

void register_fs_module(void) {
    lx_register_extension("fs");
    lx_register_module(fs_module_init);
}
