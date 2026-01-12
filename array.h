/**
 * @file array.h
 * @brief Associative array implementation with integer and string keys.
 */
#ifndef ARRAY_H
#define ARRAY_H

#include "value.h"

/** Key type discriminator for array entries. */
typedef enum { KEY_INT, KEY_STRING } KeyType;

/** Array key stored as either integer or string. */
typedef struct {
    KeyType type;
    union {
        lx_int_t i;    /**< Integer key. */
        char *s; /**< Owned string key. */
    };
} Key;

/** Key-value entry stored in an array. */
typedef struct {
    Key key;      /**< Entry key. */
    Value value;  /**< Entry value. */
} ArrayEntry;

/** Dynamic array backing store. */
struct Array {
    size_t size;         /**< Number of live entries. */
    size_t capacity;     /**< Allocated entry capacity. */
    ArrayEntry *entries; /**< Entry storage. */
    int refcount;        /**< Reference count for shared arrays. */
    int gc_mark;         /**< Mark bit used by GC. */
    struct Array *gc_next; /**< Next array in GC list. */
};

/** @return An integer key wrapper. */
Key   key_int(lx_int_t i);
/** @return A string key wrapper (owned copy). */
Key   key_string(const char *s);

/** @return A new empty array with refcount 1. */
Array *array_new(void);
/** Retain an array reference. */
void   array_retain(Array *a);
/** @return A shallow copy of entries (arrays retained). */
Array *array_copy(Array *a);
/** Release a reference to @p a. */
void   array_free(Array *a);

/** @return A copy of the value for @p k (undefined if missing). */
Value  array_get(Array *a, Key k);
/** @return Pointer to the value slot for @p k (creates if missing). */
Value *array_get_ref(Array *a, Key k);
/** Store @p v under @p k, taking ownership of @p v. */
void   array_set(Array *a, Key k, Value v);

/** @return Number of entries in @p a. */
size_t array_len(Array *a);
/** @return Next numeric index after the largest integer key. */
lx_int_t array_next_index(Array *a);

/** Remove the entry for @p k if present. */
void array_unset(Array *a, Key k);

#endif
