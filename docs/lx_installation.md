# Lx Installation and building

This document explains how to build, run, and test the Lx interpreter from source.

## Prerequisites

- A C compiler that supports C99 (e.g., `gcc` or `clang`)
- If you want the extension ext_sqlite available in Lx, install the sqlite3 library and header files*
- `make`

* To install the sqlite3 library and header files:

- on **Debian** like distribs `sudo apt install libsqlite3-dev`,
- on **RedHat** like distribs `sudo dnf install sqlite-devel` or `sudo yum install sqlite-devel`,
- on **MacOs X** with MacPorts `sudo port install sqlite3`. You may need to add -I/opt/local/include and -L/opt/local/lib in the Makefile in the LDFLAGS.

You may need to adjust parameters in the Makefile, and the rest of the C code if you dont want an extension (there is no config.h at present).

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
