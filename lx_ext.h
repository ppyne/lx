/**
 * @file lx_ext.h
 * @brief Extension API for registering functions and globals.
 */
#ifndef LX_EXT_H
#define LX_EXT_H

#include "env.h"
#include "natives.h"

/** Extension module initializer. */
typedef void (*LxModuleInit)(Env *global);

/** Register an extension name for introspection. */
void lx_register_extension(const char *name);
/** @return Number of registered extensions. */
int lx_extension_count(void);
/** @return Extension name by index, or NULL if out of range. */
const char *lx_extension_name(int index);

/** Register an extension module initializer. */
void lx_register_module(LxModuleInit init);
/** Reset registered modules and extensions. */
void lx_reset_extensions(void);
/** Invoke all registered modules with the global environment. */
void lx_init_modules(Env *global);

/** Register a native function (extension-friendly wrapper). */
void lx_register_function(const char *name, NativeFn fn);
/**
 * Register a global constant value.
 * Note: constants are not enforced by the language at runtime.
 */
void lx_register_constant(Env *global, const char *name, Value v);
/**
 * Register a global variable value.
 * Note: this uses the same binding mechanism as constants.
 */
void lx_register_variable(Env *global, const char *name, Value v);

#endif
