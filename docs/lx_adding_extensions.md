# Adding extensions to Lx

This document explains how to create and register a native C extension for Lx.

## Overview

Extensions are compiled into the Lx binary and initialized at startup.
An extension can register:

- native functions
- global constants
- global variables

The extension API lives in `lx_ext.h`.

## Minimal extension example

Create a new C file, for example `ext_hello.c`:

```c
#include "lx_ext.h"

static Value n_hello(Env *env, int argc, Value *argv) {
    (void)env; (void)argc; (void)argv;
    return value_string("hello");
}

static void my_module_init(Env *global) {
    lx_register_function("hello", n_hello);
    lx_register_constant(global, "MY_CONST", value_int(123));
    lx_register_variable(global, "greeting", value_string("hi"));
}

void register_my_module(void) {
    lx_register_module(my_module_init);
    lx_register_extension("hello");
}
```

Notes:

- `n_hello` is a native function with the signature `Value fn(Env*, int, Value*)`.
- `lx_register_function` makes it available to Lx scripts.
- Constants and variables are normal global bindings (Lx does not enforce immutability).
- `lx_register_extension` registers the extension name for `lxinfo()`.

## Wire the extension into the build

1) Add the new source file to the build:

Edit `Makefile` and append your file to `SRCS`:

```
SRCS = ... ext_hello.c
```

2) Register the module in `main.c`:

Add a forward declaration near the top:

```c
void register_my_module(void);
```

Then call it before `lx_init_modules(global)`:

```c
register_my_module();
```

3) Rebuild:

```sh
make
```

## Using the extension from Lx

```php
print(hello() . LX_EOL);    // hello
print(MY_CONST . LX_EOL);   // 123
print(greeting . LX_EOL);   // hi
```

## Common mistakes

- Forgetting to add the new `.c` file to `SRCS`
- Forgetting to call the register function in `ext_main.c`
- Returning a `Value` without using `value_*` helpers (memory ownership issues)
