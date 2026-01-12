# Predefined constants reference

Lx provides a set of predefined constants that expose information about the runtime environment, numeric limits of the current Lx binary and mathematical constants.

The constants below are always available globally as part of the Lx core and cannot be modified.

---

## Version and Environment

- **`LX_VERSION`** (`string`)  
  The version string of the running Lx interpreter.  
  Example: `"1.1.0"`

- **`LX_EOL`** (`string`)  
  The end-of-line sequence used by Lx.  
  Currently defined as a newline character (`"\n"`).

- **`LX_ENDIANNESS`** (`int`)  
  Host byte order for `blob()` numeric conversions.  
  `0` for little-endian (x86, x86_64, most ARM), `1` for big-endian (some legacy PowerPC, SPARC).  
  This indicates the byte order used when `blob()` copies raw numeric memory; the resulting byte sequence is platform-dependent.

- **`LX_ENDIAN_LITTLE`** (`int`)  
  Constant for little-endian (`0`).

- **`LX_ENDIAN_BIG`** (`int`)  
  Constant for big-endian (`1`).

---

## Integer Limits

- **`LX_INT_MAX`** (`int`)  
  The largest integer value supported by this Lx binary.

- **`LX_INT_MIN`** (`int`)  
  The smallest integer value supported by this version of Lx.

- **`LX_INT_SIZE`** (`int`)  
  The size of an integer, in bytes, in this version of Lx (controlled by `LX_INT_BITS` in `config.h`).

- **`LX_INDEX_MAX`** (`int`)  
  The maximum usable index for arrays, strings, and blobs.  
  This is the minimum of `LX_INT_MAX` and the host `size_t` limit.

These values reflect the integer representation used internally by the interpreter.

---

## Floating-Point Limits

- **`LX_FLOAT_SIZE`** (`int`)  
  The size of a floating-point value, in bytes, in this version of Lx.

- **`LX_FLOAT_DIG`** (`int`)  
  The number of decimal digits that can be rounded and returned for a floating-point number without loss of precision.

- **`LX_FLOAT_EPSILON`** (`float`)  
  The smallest positive floating-point value such that `1.0 + x != 1.0`.

- **`LX_FLOAT_MIN`** (`float`)  
  The smallest positive floating-point value supported.  
  To obtain the smallest negative floating-point value, use `-LX_FLOAT_MAX`.

- **`LX_FLOAT_MAX`** (`float`)  
  The largest floating-point value supported.

These constants describe the floating-point characteristics of the current Lx implementation.

---

## Sorting constants

- **`SORT_ASC`** (`int`)  
  Sort ascending (value `4`).

- **`SORT_DESC`** (`int`)  
  Sort descending (value `3`).

- **`SORT_REGULAR`** (`int`)  
  Regular comparisons (value `0`).

- **`SORT_NUMERIC`** (`int`)  
  Numeric comparisons (value `1`).

- **`SORT_STRING`** (`int`)  
  String comparisons (value `2`).

---

## Stream constants

- **`LX_STDIN`** (`int`)  
  Standard input stream id (`0`).

- **`LX_STDOUT`** (`int`)  
  Standard output stream id (`1`).

- **`LX_STDERR`** (`int`)  
  Standard error stream id (`2`).

---

## Mathematical constants

- **`M_PI`** (`float`)  
  Pi (3.141592653589793).

- **`M_E`** (`float`)  
  Euler's number (2.718281828459045).

- **`M_LN2`** (`float`)  
  Natural logarithm of 2.

- **`M_LN10`** (`float`)  
  Natural logarithm of 10.

- **`M_LOG2E`** (`float`)  
  Base-2 logarithm of e.

- **`M_LOG10E`** (`float`)  
  Base-10 logarithm of e.

- **`M_SQRT2`** (`float`)  
  Square root of 2.

- **`M_SQRT1_2`** (`float`)  
  Square root of 1/2.
