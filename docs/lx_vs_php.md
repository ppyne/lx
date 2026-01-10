# Lx vs PHP

This document lists differences between Lx and PHP based on the current Lx implementation.
It focuses on language capabilities, behavior, and results.

## Lx features that PHP does not have (or not in the same way)

- `undefined` and `void` are first-class values and types in Lx.
- Double-dollar variables (`$$name`) are supported without requiring `${}` syntax.
- Default parameters are evaluated in the function local environment and can reference earlier parameters directly.
- `switch (expr, strict)` optionally enables strict comparison in switch (default is weak).
- `in_array(value, array[, strict])` defaults to strict comparison in Lx.
- Built-in extensions are small and explicit; there is no implicit global function flood beyond what is registered.
- UTF-8 helpers (`glyph_count`, `glyph_at`) are built-in in the `utf8` extension.

## PHP features that Lx does not implement

Language and runtime:

- Classes, objects, interfaces, traits, inheritance, namespaces.
- Exceptions and `try/catch/finally`.
- References (`&`) and copy-on-write semantics for user code.
- Generators (`yield`), iterators, SPL.
- Attributes/annotations, reflection, type declarations, union types.
- Anonymous functions and closures.
- `include`/`require` operators (Lx uses functions `include`, `include_once`).
- `list()` and `[]` destructuring in PHP (Lx supports `[$a, $b] = ...` but not PHP `list()`).
- Ternary shorthand (`?:`) and null coalescing (`??`) are not present.
- `foreach` by reference and key/value unpacking are not present.

Standard library and extensions:

- The majority of PHP standard library and extensions are not implemented.
- Common hash functions like `md5()` and `sha*()` are not present; Lx provides `blake2b()` instead.
- No native multibyte string library; only the `utf8` helper extension described above.
- No database drivers, curl, sockets, GD, or other large PHP subsystems.

Execution model:

- No opcode VM or JIT.
- No INI configuration or runtime `ini_*` APIs.

## Behavioral differences where both have a similar feature

Strings:

- Lx supports double-quoted interpolation for `$name` and `${expr}`.
- Single-quoted strings only interpret `\\` and `\'`.
- `print`/`printf` return `void` in Lx (no return value).

Arrays:

- Lx arrays are associative and support int/string keys, but strict array equality is identity-based.
- `switch` strict mode and strict equality compare arrays by identity, not by deep equality.
- `$array[] = value` appends by next numeric index in Lx.

Functions and scope:

- Lx functions are local-scope by default. Use `global` to access/modify global variables.
- Parameters may be declared without `$` in Lx.
- Missing arguments default to `null` unless a default value is provided.

Comparison:

- Lx has both weak and strict comparisons (`==` / `===`), but some APIs default differently:
  - `switch` uses weak comparison unless `switch (expr, true)` is used.
  - `in_array` defaults to strict comparison, optional `strict=false` for weak.

Errors:

- Lx stops execution on errors and prints a single-line error message.
- Many PHP warnings/notices do not exist in Lx; error reporting is simpler.

I/O and includes:

- `include` / `include_once` are functions in Lx.
- `__FILE__` and `__DIR__` are resolved paths in Lx (symlinks resolved).
- `json_decode()` returns `undefined` on invalid JSON; `is_json()` can be used to validate without errors.

## Notes

- Lx is intentionally small and focused; PHP is a large, general-purpose runtime.
- When in doubt, prefer the Lx user manual and function reference for exact behavior.
