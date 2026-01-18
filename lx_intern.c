/**
 * @file lx_intern.c
 * @brief String interning implementation.
 */
#include "lx_intern.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct InternNode {
    char *str;
    uint32_t hash;
    struct InternNode *next;
} InternNode;

static InternNode **g_buckets = NULL;
static size_t g_bucket_count = 0;
static size_t g_count = 0;

static uint32_t hash_bytes(const char *s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}

static void intern_init(void) {
    if (g_buckets) return;
    g_bucket_count = 256;
    g_buckets = (InternNode **)calloc(g_bucket_count, sizeof(InternNode *));
    g_count = 0;
}

static void intern_rehash(size_t new_count) {
    InternNode **nb = (InternNode **)calloc(new_count, sizeof(InternNode *));
    if (!nb) return;

    for (size_t i = 0; i < g_bucket_count; i++) {
        InternNode *n = g_buckets[i];
        while (n) {
            InternNode *next = n->next;
            size_t idx = n->hash % new_count;
            n->next = nb[idx];
            nb[idx] = n;
            n = next;
        }
    }
    free(g_buckets);
    g_buckets = nb;
    g_bucket_count = new_count;
}

const char *lx_intern(const char *s) {
    if (!s) return NULL;
    char *dup = strdup(s);
    if (!dup) return NULL;
    return lx_intern_take(dup);
}

char *lx_intern_take(char *s) {
    if (!s) return NULL;
    intern_init();
    if (!g_buckets) return s;

    uint32_t h = hash_bytes(s);
    size_t idx = h % g_bucket_count;
    for (InternNode *n = g_buckets[idx]; n; n = n->next) {
        if (n->hash == h && strcmp(n->str, s) == 0) {
            free(s);
            return n->str;
        }
    }

    if (g_count * 4 >= g_bucket_count * 3) {
        intern_rehash(g_bucket_count * 2);
        idx = h % g_bucket_count;
    }

    InternNode *node = (InternNode *)malloc(sizeof(InternNode));
    if (!node) return s;
    node->str = s;
    node->hash = h;
    node->next = g_buckets[idx];
    g_buckets[idx] = node;
    g_count++;
    return s;
}

int lx_intern_is_interned(const char *s) {
    if (!s || !g_buckets) return 0;
    uint32_t h = hash_bytes(s);
    size_t idx = h % g_bucket_count;
    for (InternNode *n = g_buckets[idx]; n; n = n->next) {
        if (n->str == s) return 1;
    }
    return 0;
}
