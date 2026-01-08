/**
 * @file array.c
 * @brief Array implementation.
 */
#include "array.h"
#include "lx_error.h"
#include "gc.h"
#include <stdlib.h>
#include <string.h>

static void key_free(Key k) {
    if (k.type == KEY_STRING) free(k.s);
}
static Key key_copy(Key k) {
    Key out = k;
    if (k.type == KEY_STRING) out.s = strdup(k.s ? k.s : "");
    return out;
}
static int key_eq(Key a, Key b) {
    if (a.type != b.type) return 0;
    if (a.type == KEY_INT) return a.i == b.i;
    return strcmp(a.s ? a.s : "", b.s ? b.s : "") == 0;
}

static int array_contains_inner(Array *hay, Array *needle, Array ***visited, int *count, int *cap) {
    if (!hay || !needle) return 0;
    if (hay == needle) return 1;
    for (int i = 0; i < *count; i++) {
        if ((*visited)[i] == hay) return 0;
    }
    if (*count >= *cap) {
        int ncap = (*cap == 0) ? 8 : (*cap * 2);
        Array **nv = (Array **)realloc(*visited, (size_t)ncap * sizeof(Array *));
        if (!nv) return 0;
        *visited = nv;
        *cap = ncap;
    }
    (*visited)[(*count)++] = hay;
    for (int i = 0; i < hay->size; i++) {
        Value v = hay->entries[i].value;
        if (v.type == VAL_ARRAY && v.a) {
            if (array_contains_inner(v.a, needle, visited, count, cap)) return 1;
        }
    }
    return 0;
}

static int array_contains(Array *hay, Array *needle) {
    Array **visited = NULL;
    int count = 0;
    int cap = 0;
    int found = array_contains_inner(hay, needle, &visited, &count, &cap);
    free(visited);
    return found;
}

Key key_int(int i) { Key k; k.type=KEY_INT; k.i=i; return k; }
Key key_string(const char *s) { Key k; k.type=KEY_STRING; k.s=strdup(s?s:""); return k; }

Array *array_new(void) {
    Array *a = (Array*)calloc(1, sizeof(Array));
    if (a) {
        a->refcount = 1;
        gc_register_array(a);
    }
    return a;
}

void array_retain(Array *a) {
    if (a) a->refcount++;
}

static void ensure(Array *a, int need) {
    if (a->capacity >= need) return;
    int cap = a->capacity ? a->capacity : 8;
    while (cap < need) cap *= 2;
    ArrayEntry *ne = (ArrayEntry*)realloc(a->entries, cap * sizeof(ArrayEntry));
    if (!ne) return;
    a->entries = ne;
    a->capacity = cap;
}

int array_len(Array *a) { return a ? a->size : 0; }

Value array_get(Array *a, Key k) {
    if (!a) { key_free(k); return value_undefined(); }
    for (int i=0;i<a->size;i++) {
        if (key_eq(a->entries[i].key, k)) {
            key_free(k);
            return value_copy(a->entries[i].value);
        }
    }
    key_free(k);
    return value_undefined();
}

void array_set(Array *a, Key k, Value v) {
    if (!a) { key_free(k); value_free(v); return; }
    if (v.type == VAL_ARRAY && v.a) {
        if (array_contains(v.a, a)) {
            lx_set_error(LX_ERR_CYCLE, 0, 0, "cyclic array reference");
            key_free(k);
            value_free(v);
            return;
        }
    }
    for (int i=0;i<a->size;i++) {
        if (key_eq(a->entries[i].key, k)) {
            key_free(k);
            value_free(a->entries[i].value);
            a->entries[i].value = v;
            return;
        }
    }
    ensure(a, a->size+1);
    a->entries[a->size].key = key_copy(k);
    a->entries[a->size].value = v;
    a->size++;
    key_free(k);
}

Array *array_copy(Array *a) {
    if (!a) return NULL;
    Array *b = array_new();
    ensure(b, a->size);
    for (int i=0;i<a->size;i++) {
        b->entries[i].key = key_copy(a->entries[i].key);
        b->entries[i].value = value_copy(a->entries[i].value);
    }
    b->size = a->size;
    return b;
}

void array_free(Array *a) {
    if (!a) return;
    if (--a->refcount > 0) return;
    gc_unregister_array(a);
    for (int i=0;i<a->size;i++) {
        key_free(a->entries[i].key);
        value_free(a->entries[i].value);
    }
    free(a->entries);
    free(a);
}

void array_unset(Array *a, Key k) {
    if (!a) {
        key_free(k);
        return;
    }

    for (int i = 0; i < a->size; i++) {
        if (key_eq(a->entries[i].key, k)) {

            /* Free key and value. */
            key_free(a->entries[i].key);
            value_free(a->entries[i].value);

            /* Shift remaining entries down. */
            for (int j = i + 1; j < a->size; j++) {
                a->entries[j - 1] = a->entries[j];
            }

            a->size--;
            key_free(k);
            return;
        }
    }

    /* Missing key: no-op. */
    key_free(k);
}

Value *array_get_ref(Array *a, Key k) {
    if (!a) { key_free(k); return NULL; }

    for (int i = 0; i < a->size; i++) {
        if (key_eq(a->entries[i].key, k)) {
            key_free(k);
            return &a->entries[i].value;
        }
    }

    /* Missing key: create slot. */
    ensure(a, a->size + 1);
    a->entries[a->size].key = key_copy(k);
    a->entries[a->size].value = value_undefined();
    key_free(k);

    return &a->entries[a->size++].value;
}
