/**
 * @file env.c
 * @brief Environment implementation.
 */
#include "env.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    Value value;
} Binding;

struct Env {
    struct Env *parent;
    Binding *items;
    int count;
    int cap;
    char **globals;
    int global_count;
    int global_cap;
};

static int global_index(Env *e, const char *name);
int env_is_global(Env *e, const char *name);

Env *env_new(Env *parent){
    Env *e = (Env*)calloc(1, sizeof(Env));
    e->parent = parent;
    return e;
}

static void env_items_free(Env *e){
    for (int i=0;i<e->count;i++){
        free(e->items[i].name);
        value_free(e->items[i].value);
    }
    free(e->items);
}

void env_free(Env *e){
    if (!e) return;
    env_items_free(e);
    for (int i = 0; i < e->global_count; i++) {
        free(e->globals[i]);
    }
    free(e->globals);
    free(e);
}

static int find_local(Env *e, const char *name){
    for (int i=0;i<e->count;i++){
        if (strcmp(e->items[i].name, name)==0) return i;
    }
    return -1;
}

int env_has(Env *e, const char *name){
    if (!e) return 0;
    if (env_is_global(e, name)) {
        Env *root = e;
        while (root->parent) root = root->parent;
        return find_local(root, name) >= 0;
    }
    return find_local(e, name) >= 0;
}

static void ensure(Env *e, int need);

Value env_get(Env *e, const char *name){
    if (!e) return value_undefined();
    if (env_is_global(e, name)) {
        Env *root = e;
        while (root->parent) root = root->parent;
        int idx = find_local(root, name);
        if (idx >= 0) return value_copy(root->items[idx].value);
        return value_undefined();
    }
    int idx = find_local(e, name);
    if (idx >= 0) return value_copy(e->items[idx].value);
    return value_undefined();
}

Value *env_get_ref(Env *e, const char *name){
    if (!e) return NULL;
    if (env_is_global(e, name)) {
        Env *root = e;
        while (root->parent) root = root->parent;
        int idx = find_local(root, name);
        if (idx >= 0) return &root->items[idx].value;
        ensure(root, root->count + 1);
        root->items[root->count].name = strdup(name);
        root->items[root->count].value = value_undefined();
        return &root->items[root->count++].value;
    }
    int idx = find_local(e, name);
    if (idx >= 0) return &e->items[idx].value;
    ensure(e, e->count+1);
    e->items[e->count].name = strdup(name);
    e->items[e->count].value = value_undefined();
    return &e->items[e->count++].value;
}

static void ensure(Env *e, int need){
    if (e->cap >= need) return;
    int cap = e->cap ? e->cap : 16;
    while (cap < need) cap *= 2;
    Binding *nb = (Binding*)realloc(e->items, cap*sizeof(Binding));
    if (!nb) return;
    e->items = nb;
    e->cap = cap;
}

void env_set(Env *e, const char *name, Value v){
    if (!e) { value_free(v); return; }
    if (env_is_global(e, name)) {
        Env *root = e;
        while (root->parent) root = root->parent;
        int idx = find_local(root, name);
        if (idx >= 0){
            value_free(root->items[idx].value);
            root->items[idx].value = v;
            return;
        }
        ensure(root, root->count+1);
        root->items[root->count].name = strdup(name);
        root->items[root->count].value = v;
        root->count++;
        return;
    }
    int idx = find_local(e, name);
    if (idx >= 0){
        value_free(e->items[idx].value);
        e->items[idx].value = v;
        return;
    }
    ensure(e, e->count+1);
    e->items[e->count].name = strdup(name);
    e->items[e->count].value = v;
    e->count++;
}

void env_unset(Env *e, const char *name){
    if (!e) return;
    if (env_is_global(e, name)) {
        Env *root = e;
        while (root->parent) root = root->parent;
        int idx = find_local(root, name);
        if (idx < 0) return;
        free(root->items[idx].name);
        value_free(root->items[idx].value);
        for (int i=idx; i<root->count-1; i++){
            root->items[i] = root->items[i+1];
        }
        root->count--;
        return;
    }
    int idx = find_local(e, name);
    if (idx < 0) return;
    free(e->items[idx].name);
    value_free(e->items[idx].value);
    for (int i=idx; i<e->count-1; i++){
        e->items[i] = e->items[i+1];
    }
    e->count--;
}

void env_visit(Env *e, EnvVisitFn fn, void *ctx) {
    if (!fn) return;
    for (Env *cur = e; cur; cur = cur->parent) {
        for (int i = 0; i < cur->count; i++) {
            fn(cur->items[i].name, &cur->items[i].value, ctx);
        }
    }
}

static int global_index(Env *e, const char *name) {
    for (int i = 0; i < e->global_count; i++) {
        if (strcmp(e->globals[i], name) == 0) return i;
    }
    return -1;
}

void env_add_global(Env *e, const char *name) {
    if (!e || !name || !*name) return;
    if (global_index(e, name) >= 0) return;
    if (e->global_cap <= e->global_count) {
        int cap = e->global_cap ? e->global_cap * 2 : 8;
        while (cap <= e->global_count) cap *= 2;
        char **nn = (char **)realloc(e->globals, (size_t)cap * sizeof(char *));
        if (!nn) return;
        e->globals = nn;
        e->global_cap = cap;
    }
    e->globals[e->global_count++] = strdup(name);
}

int env_is_global(Env *e, const char *name) {
    if (!e || !name || !*name) return 0;
    return global_index(e, name) >= 0;
}
