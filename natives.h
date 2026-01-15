/**
 * @file natives.h
 * @brief Native function registry and standard library setup.
 */
#ifndef NATIVES_H
#define NATIVES_H

#include <stdio.h>
#include <stddef.h>

#include "value.h"
#include "env.h"

/** Native function signature. */
typedef Value (*NativeFn)(Env *env, int argc, Value *argv);
typedef void (*LxOutputFn)(const char *data, size_t len);

/** Register or replace a native function. */
void     register_function(const char *name, NativeFn fn);
/** Look up a native function by name. */
NativeFn find_function(const char *name);

/** Override the output stream used by print/printf/var_dump/print_r. */
void lx_set_output(FILE *f);
FILE *lx_get_output(void);
void lx_set_output_cb(LxOutputFn fn);

/** Install the minimal standard library into the registry. */
void install_stdlib(void);

#endif
