# Lx — Scripting Language

Lx is a small, interpreted, dynamically typed scripting language with a PHP-like surface syntax.  
It is a minimal runtime interpreter with no bytecode or VM layer.

<p align="center"><img src="docs/lx_mascot.svg" width="128" alt="Lx mascot"></p>

## Technical overview

Lx is a small interpreted language implemented in C. The front‑end is a hand‑written lexer and
recursive‑descent parser that builds an AST for expressions, statements, and control flow. The runtime
evaluates the AST directly (no bytecode), with a dynamic `Value` type supporting int, float, bool,
string, array, null/undefined/void. Arrays are associative (int/string keys) and use reference
counting with a periodic mark‑and‑sweep pass to break cycles. The environment model is lexical with
explicit `global` declarations. Built‑in functions and extensions are registered through a small C
extension API and exposed at startup.

## License

BSD 3-Clause License. See [`LICENSE`](LICENSE) for details.

## Documentation

- [`docs/lx_user_manual.md`](docs/lx_user_manual.md) — language reference
- [`docs/lx_functions_reference.md`](docs/lx_functions_reference.md) — functions reference
- [`docs/llx_predefined_constants.md`](docs/lx_predefined_constants.md) — predefined constants reference
- [`docs/lx_installation.md`](docs/lx_installation.md) — build, run, and test instructions
- [`docs/lx_adding_extensions.md`](docs/lx_adding_extensions.md) — how to add your own extensions

## Quick Start

```sh
cd lx
make
./lx tests/core/variables.lx
```

## Tests

```sh
make test
```
