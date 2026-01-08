# Lx Installation and building

This document explains how to build, run, and test the Lx interpreter from source.

## Prerequisites

- A C compiler that supports C99 (e.g., `gcc` or `clang`)
- `make`

## Build

From the project root:

```sh
make
```

This produces the `lx` binary in the project root.

## Run a script

```sh
./lx path/to/script.lx
```

Example:

```sh
./lx tests/core/variables.lx
```

## Run tests

```sh
make test
```

The test runner executes `tests/run_tests.sh`.

## Clean build artifacts

```sh
make clean
```
