/**
 * @file lx_ext.c
 * @brief Extension registration helpers.
 */
#include "lx_ext.h"
#include <stdlib.h>
#include <string.h>

static LxModuleInit *g_mods = NULL;
static int g_mod_count = 0;
static int g_mod_cap = 0;

static char **g_ext_names = NULL;
static int g_ext_count = 0;
static int g_ext_cap = 0;

static void ensure_mods(int need) {
    if (g_mod_cap >= need) return;
    int cap = g_mod_cap ? g_mod_cap : 8;
    while (cap < need) cap *= 2;
    LxModuleInit *nm = (LxModuleInit *)realloc(g_mods, (size_t)cap * sizeof(LxModuleInit));
    if (!nm) return;
    g_mods = nm;
    g_mod_cap = cap;
}

static void ensure_exts(int need) {
    if (g_ext_cap >= need) return;
    int cap = g_ext_cap ? g_ext_cap : 8;
    while (cap < need) cap *= 2;
    char **nn = (char **)realloc(g_ext_names, (size_t)cap * sizeof(char *));
    if (!nn) return;
    g_ext_names = nn;
    g_ext_cap = cap;
}

void lx_register_extension(const char *name) {
    if (!name || !*name) return;
    for (int i = 0; i < g_ext_count; i++) {
        if (strcmp(g_ext_names[i], name) == 0) return;
    }
    ensure_exts(g_ext_count + 1);
    g_ext_names[g_ext_count++] = strdup(name);
}

int lx_extension_count(void) {
    return g_ext_count;
}

const char *lx_extension_name(int index) {
    if (index < 0 || index >= g_ext_count) return NULL;
    return g_ext_names[index];
}

void lx_register_module(LxModuleInit init) {
    if (!init) return;
    ensure_mods(g_mod_count + 1);
    g_mods[g_mod_count++] = init;
}

void lx_init_modules(Env *global) {
    for (int i = 0; i < g_mod_count; i++) {
        g_mods[i](global);
    }
}

void lx_register_function(const char *name, NativeFn fn) {
    register_function(name, fn);
}

void lx_register_constant(Env *global, const char *name, Value v) {
    if (!global || !name) { value_free(v); return; }
    env_set(global, name, v);
}

void lx_register_variable(Env *global, const char *name, Value v) {
    if (!global || !name) { value_free(v); return; }
    env_set(global, name, v);
}
