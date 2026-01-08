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
};

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
    free(e);
}

static int find_local(Env *e, const char *name){
    for (int i=0;i<e->count;i++){
        if (strcmp(e->items[i].name, name)==0) return i;
    }
    return -1;
}

int env_has(Env *e, const char *name){
    for (Env *cur=e; cur; cur=cur->parent){
        if (find_local(cur, name) >= 0) return 1;
    }
    return 0;
}

static void ensure(Env *e, int need);

Value env_get(Env *e, const char *name){
    for (Env *cur=e; cur; cur=cur->parent){
        int idx = find_local(cur, name);
        if (idx >= 0) return value_copy(cur->items[idx].value);
    }
    return value_undefined();
}

Value *env_get_ref(Env *e, const char *name){
    for (Env *cur=e; cur; cur=cur->parent){
        int idx = find_local(cur, name);
        if (idx >= 0) return &cur->items[idx].value;
    }
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
    for (Env *cur = e; cur; cur = cur->parent) {
        int idx = find_local(cur, name);
        if (idx >= 0){
            value_free(cur->items[idx].value);
            cur->items[idx].value = v;
            return;
        }
    }
    ensure(e, e->count+1);
    e->items[e->count].name = strdup(name);
    e->items[e->count].value = v;
    e->count++;
}

void env_unset(Env *e, const char *name){
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
