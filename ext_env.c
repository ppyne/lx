/**
 * @file ext_env.c
 * @brief Environment variables extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>

extern char **environ;

static char *dup_range(const char *s, size_t n) {
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static Value n_env_get(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1 || argv[0].type != VAL_STRING) return value_undefined();
    const char *name = argv[0].s ? argv[0].s : "";
    const char *val = getenv(name);
    if (!val) {
        if (argc >= 2) return value_to_string(argv[1]);
        return value_undefined();
    }
    return value_string(val);
}

static Value n_env_set(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *name = argv[0].s ? argv[0].s : "";
    Value sv = value_to_string(argv[1]);
    const char *val = sv.s ? sv.s : "";
    int ok = (setenv(name, val, 1) == 0);
    value_free(sv);
    return value_bool(ok);
}

static Value n_env_unset(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_bool(0);
    const char *name = argv[0].s ? argv[0].s : "";
    return value_bool(unsetenv(name) == 0);
}

static Value n_env_list(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    Value out = value_array();
    if (argc != 0) return out;
    if (!environ) return out;
    for (char **p = environ; *p; p++) {
        const char *entry = *p;
        const char *eq = strchr(entry, '=');
        if (!eq) continue;
        size_t nlen = (size_t)(eq - entry);
        char *name = dup_range(entry, nlen);
        const char *val = eq + 1;
        if (!name) continue;
        array_set(out.a, key_string(name), value_string(val));
        free(name);
    }
    return out;
}

static void env_module_init(Env *global){
    lx_register_function("env_get", n_env_get);
    lx_register_function("env_set", n_env_set);
    lx_register_function("env_unset", n_env_unset);
    lx_register_function("env_list", n_env_list);
    (void)global;
}

void register_env_module(void) {
    lx_register_extension("env");
    lx_register_module(env_module_init);
}
