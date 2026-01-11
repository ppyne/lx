# Lx Installation and building

This document explains how to build, run, and test the Lx interpreter from source.

## Prerequisites

- A C compiler that supports C99 (e.g., `gcc` or `clang`)
- If you want the SQLite extension available in Lx, install the sqlite3 library and header files*
- `make`

* To install the sqlite3 library and header files:

- on **Debian**-like distros: `sudo apt install libsqlite3-dev`
- on **RedHat**-like distros: `sudo dnf install sqlite-devel` or `sudo yum install sqlite-devel`
- on **macOS** with MacPorts: `sudo port install sqlite3` (you may need to add `-I/opt/local/include` and `-L/opt/local/lib` to `LDFLAGS`)

If you want to enable or disable extensions, edit `config.h` before building. Each
`LX_ENABLE_*` macro can be set to `1` (enabled) or `0` (disabled). The build
will only compile the enabled extensions, and tests for disabled extensions are
skipped.

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
