# Lx — Scripting Language

Lx is a small, interpreted, dynamically typed scripting language with a PHP-like surface syntax.  
It is designed for embedded use cases and a minimal, predictable runtime.

<p align="center"><img src="docs/lx_mascot.svg" width="128" alt="Lx mascot"></p>

## License

BSD 3-Clause License. See `LICENSE` for details.

## Documentation

- [`docs/lx_user_manual.md`](docs/lx_user_manual.md) — language reference and built-in APIs
- [`docs/lx_installation.md`](docs/lx_installation.md) — build, run, and test instructions
- [`docs/lx_adding_extensions.md`](docs/lx_adding_extensions.md) — how to add native extensions

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
