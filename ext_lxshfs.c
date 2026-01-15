/**
 * @file ext_lxshfs.c
 * @brief Filesystem extension module backed by Lx shell.
 */
#include "lx_ext.h"
#include "lxsh_fs.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>

static const LxShFsOps *get_ops(void) {
    return lxsh_get_fs_ops();
}

static Value n_sys_get_temp_dir(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_string("");
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->temp_dir) return value_string("");
    return value_string(ops->temp_dir());
}

static Value n_tempnam(Env *env, int argc, Value *argv){
    (void)env;
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->tempnam) return value_undefined();
    const char *prefix = "lx";
    if (argc == 1 && argv[0].type == VAL_STRING && argv[0].s && *argv[0].s) {
        prefix = argv[0].s;
    } else if (argc != 0 && argc != 1) {
        return value_undefined();
    }
    char path[128];
    if (!ops->tempnam(prefix, path, sizeof(path))) {
        return value_undefined();
    }
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
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->read_file) return value_undefined();
    char *buf = NULL;
    size_t len = 0;
    if (!ops->read_file(path, &buf, &len, NULL)) return value_undefined();
    if (want_blob) {
        Value out = value_blob_n((const unsigned char *)buf, len);
        free(buf);
        return out;
    }
    size_t out_len = len;
    char *nul = memchr(buf, 0, len);
    if (nul) out_len = (size_t)(nul - buf);
    Value out = value_string_n(buf, out_len);
    free(buf);
    return out;
}

static Value n_file_put_contents(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_int(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->write_file) return value_int(0);
    const char *path = argv[0].s ? argv[0].s : "";
    size_t n = 0;
    if (argv[1].type == VAL_BLOB && argv[1].blob) {
        n = argv[1].blob->len;
        if (!ops->write_file(path, argv[1].blob->data, n)) {
            return value_int(0);
        }
    } else {
        Value sv = value_to_string(argv[1]);
        const char *s = sv.s ? sv.s : "";
        n = strlen(s);
        int ok = ops->write_file(path, (const unsigned char *)s, n);
        value_free(sv);
        if (!ok) return value_int(0);
    }
    return value_int((int)n);
}

static Value n_file_exists(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->file_exists) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->file_exists(path));
}

static Value n_file_size(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->file_size) return value_undefined();
    const char *path = argv[0].s ? argv[0].s : "";
    size_t size = 0;
    if (!ops->file_size(path, &size)) return value_undefined();
    return value_int((int)size);
}

static Value n_is_dir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->is_dir) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->is_dir(path));
}

static Value n_is_file(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->is_file) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->is_file(path));
}

static Value n_mkdir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->mkdir) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->mkdir(path));
}

static Value n_rmdir(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->rmdir) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->rmdir(path));
}

static Value n_unlink(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->unlink) return value_bool(0);
    const char *path = argv[0].s ? argv[0].s : "";
    return value_bool(ops->unlink(path));
}

static Value n_copy(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->copy) return value_bool(0);
    const char *src = argv[0].s ? argv[0].s : "";
    const char *dst = argv[1].s ? argv[1].s : "";
    return value_bool(ops->copy(src, dst));
}

static Value n_rename(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING || argv[1].type != VAL_STRING) {
        return value_bool(0);
    }
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->rename) return value_bool(0);
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
            int ok = ops->rename(src, buf);
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
            int ok = ops->rename(src, buf);
            free(buf);
            return value_bool(ok);
        }
    }
    return value_bool(ops->rename(src, dst));
}

static Value n_chmod(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 2) return value_bool(0);
    return value_bool(0);
}

static Value n_pwd(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_string("");
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->pwd) return value_string("");
    char buf[128];
    if (!ops->pwd(buf, sizeof(buf))) return value_string("");
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
        else {
            dirname = (char *)malloc(dlen + 1);
            if (dirname) {
                memcpy(dirname, path, dlen);
                dirname[dlen] = '\0';
            }
        }
    } else {
        dirname = strdup(".");
    }

    const char *dot = strrchr(base, '.');
    char *extension = NULL;
    char *filename = NULL;
    if (dot && dot != base) {
        extension = strdup(dot + 1);
        filename = (char *)malloc((size_t)(dot - base) + 1);
        if (filename) {
            memcpy(filename, base, (size_t)(dot - base));
            filename[dot - base] = '\0';
        }
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
    const LxShFsOps *ops = get_ops();
    if (!ops || !ops->list_dir) return out;
    const char *path = argv[0].s ? argv[0].s : "";
    char **names = NULL;
    size_t count = 0;
    if (!ops->list_dir(path, &names, &count)) return out;
    qsort(names, count, sizeof(char *), name_cmp);
    for (size_t i = 0; i < count; i++) {
        array_set(out.a, key_int((int)i), value_string(names[i] ? names[i] : ""));
        free(names[i]);
    }
    free(names);
    return out;
}

static void lxshfs_module_init(Env *global){
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

void register_lxshfs_module(void) {
    lx_register_extension("lxshfs");
    lx_register_module(lxshfs_module_init);
}
