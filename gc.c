/**
 * @file gc.c
 * @brief Mark-and-sweep collector for arrays.
 */
#include "gc.h"
#include "array.h"
#include "env.h"
#include "value.h"
#include <stdlib.h>

static Array *g_gc_head = NULL;
static int g_gc_count = 0;
static int g_gc_threshold = 1024;

void gc_register_array(Array *a) {
    if (!a) return;
    a->gc_next = g_gc_head;
    g_gc_head = a;
    g_gc_count++;
}

void gc_unregister_array(Array *a) {
    if (!a) return;
    Array *prev = NULL;
    Array *cur = g_gc_head;
    while (cur) {
        if (cur == a) {
            if (prev) prev->gc_next = cur->gc_next;
            else g_gc_head = cur->gc_next;
            a->gc_next = NULL;
            g_gc_count--;
            return;
        }
        prev = cur;
        cur = cur->gc_next;
    }
}

static void gc_mark_array(Array *a) {
    if (!a || a->gc_mark) return;
    a->gc_mark = 1;
    for (int i = 0; i < a->size; i++) {
        Value v = a->entries[i].value;
        if (v.type == VAL_ARRAY && v.a) gc_mark_array(v.a);
    }
}

static void gc_mark_value(Value v) {
    if (v.type == VAL_ARRAY && v.a) gc_mark_array(v.a);
}

static void gc_mark_binding(const char *name, Value *val, void *ctx) {
    (void)name;
    (void)ctx;
    if (!val) return;
    gc_mark_value(*val);
}

static void gc_key_free(Key k) {
    if (k.type == KEY_STRING) free(k.s);
}

static void gc_release_value(Value v) {
    if (v.type == VAL_STRING) {
        free(v.s);
    } else if (v.type == VAL_ARRAY && v.a) {
        if (v.a->refcount > 0) v.a->refcount--;
    }
}

static void gc_free_array(Array *a) {
    if (!a) return;
    for (int i = 0; i < a->size; i++) {
        gc_key_free(a->entries[i].key);
        gc_release_value(a->entries[i].value);
    }
    free(a->entries);
    free(a);
}

void gc_collect(Env *root) {
    for (Array *cur = g_gc_head; cur; cur = cur->gc_next) {
        cur->gc_mark = 0;
    }

    if (root) env_visit(root, gc_mark_binding, NULL);

    Array *prev = NULL;
    Array *cur = g_gc_head;
    while (cur) {
        Array *next = cur->gc_next;
        if (!cur->gc_mark) {
            if (prev) prev->gc_next = next;
            else g_gc_head = next;
            g_gc_count--;
            cur->gc_next = NULL;
            gc_free_array(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }

    if (g_gc_count > g_gc_threshold) g_gc_threshold = g_gc_count * 2;
    if (g_gc_threshold < 1024) g_gc_threshold = 1024;
}

void gc_maybe_collect(Env *root) {
    if (g_gc_count > g_gc_threshold) gc_collect(root);
}

int gc_array_count(void) {
    return g_gc_count;
}
