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

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static Value n_file_get_contents(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
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
    Value out = value_string_n(buf, readn);
    free(buf);
    return out;
}

static Value n_file_put_contents(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_int(0);
    const char *path = argv[0].s ? argv[0].s : "";
    Value sv = value_to_string(argv[1]);
    const char *s = sv.s ? sv.s : "";
    FILE *f = fopen(path, "wb");
    if (!f) { value_free(sv); return value_int(0); }
    size_t n = fwrite(s, 1, strlen(s), f);
    fclose(f);
    value_free(sv);
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
    lx_register_function("pathinfo", n_pathinfo);
    lx_register_function("list_dir", n_list_dir);
    (void)global;
}

void register_fs_module(void) {
    lx_register_extension("fs");
    lx_register_module(fs_module_init);
}
