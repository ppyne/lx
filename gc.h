/**
 * @file gc.h
 * @brief Mark-and-sweep collector for arrays.
 */
#ifndef GC_H
#define GC_H

#include "env.h"

/** Register a newly created array with the GC. */
void gc_register_array(Array *a);
/** Unregister a destroyed array from the GC. */
void gc_unregister_array(Array *a);

/** Run a full GC collection using @p root as the environment root. */
void gc_collect(Env *root);
/** Trigger GC when thresholds are exceeded. */
void gc_maybe_collect(Env *root);

/** @return Number of arrays currently tracked by the GC. */
int gc_array_count(void);

#endif
