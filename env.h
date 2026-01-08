/**
 * @file env.h
 * @brief Variable environment with lexical parent chaining.
 */
#ifndef ENV_H
#define ENV_H

#include "value.h"

/** Opaque environment type. */
typedef struct Env Env;

/** Create a new environment with an optional @p parent. */
Env  *env_new(Env *parent);
/** Free an environment and all owned bindings. */
void  env_free(Env *e);

/** @return Non-zero if @p name exists in this environment chain. */
int   env_has(Env *e, const char *name);
/** @return A copy of the value for @p name, or VAL_UNDEFINED if missing. */
Value env_get(Env *e, const char *name);
/**
 * @return Pointer to the binding for @p name.
 * Creates a local binding if none exists in the chain.
 */
Value *env_get_ref(Env *e, const char *name);
/** Set @p name to @p v, taking ownership of @p v. */
void  env_set(Env *e, const char *name, Value v);
/** Remove @p name from the current scope if present. */
void  env_unset(Env *e, const char *name);

/** Callback used by env_visit. */
typedef void (*EnvVisitFn)(const char *name, Value *value, void *ctx);
/** Visit all bindings in the environment chain. */
void env_visit(Env *e, EnvVisitFn fn, void *ctx);

#endif
