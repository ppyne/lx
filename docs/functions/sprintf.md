# sprintf

Format string

Domain: Output and formatting

---

### Description

`sprintf(format, ...args) : string`

Builds and returns a formatted string.

The format string controls how the given arguments are converted and inserted into the resulting string (see **Format specification** below).

### Parameters

- **`format`**: The format string.
- **`...args`**: Values to format into the string.

### Return Values

Returns a string.

### Examples

```php
$s = sprintf("The result is: %.2f", 1.234);
print($s . "\n");

/* Will output:
The result is: 1.23
*/
```

### Format specification

#### Syntax

Each format specifier in `sprintf` and `printf` follows this general form:

```
%[flags][width][.precision]specifier
```

Not all components are required, and they may be combined depending on the specifier.

---

#### Specifier components

##### Flags

Flags modify the formatting behavior:

- `0`  
  Pads the result with leading zeros instead of spaces.

Example:

```php
sprintf("%05d", 42);   // "00042"
```

---

##### Width

The width specifies the **minimum number of characters** to output.

If the formatted value is shorter, it is padded (by default with spaces, or with zeros if the `0` flag is used).

Example:

```php
sprintf("%5d", 42);   // "   42"
sprintf("%05d", 42);  // "00042"
```

---

##### Precision

Precision is introduced by a dot (`.`) and has different meanings depending on the specifier:

- For floating-point numbers, it specifies the number of digits after the decimal point.
- For integers, it specifies the minimum number of digits.
- For strings, it specifies the maximum number of characters.

Examples:

```php
sprintf("%.2f", 3.14159);  // "3.14"
sprintf("%.3d", 7);        // "007"
sprintf("%.4s", "abcdef"); // "abcd"
```

---

#### Supported format specifiers

| Specifier | Description                                                                                                                            |
| ---------:| -------------------------------------------------------------------------------------------------------------------------------------- |
| `%s`      | String                                                                                                                                 |
| `%c`      | Character. The argument may be the first character of a string or an integer interpreted as an ASCII code. An empty string is ignored. |
| `%d`      | Signed decimal integer                                                                                                                 |
| `%i`      | Signed integer                                                                                                                         |
| `%u`      | Unsigned integer                                                                                                                       |
| `%x`      | Unsigned hexadecimal integer (lowercase)                                                                                               |
| `%X`      | Unsigned hexadecimal integer (uppercase)                                                                                               |
| `%o`      | Unsigned octal integer                                                                                                                 |
| `%f`      | Floating-point number                                                                                                                  |
| `%F`      | Floating-point number                                                                                                                  |
| `%e`      | Scientific notation (lowercase)                                                                                                        |
| `%E`      | Scientific notation (uppercase)                                                                                                        |
| `%g`      | Shortest representation (`%f` or `%e`)                                                                                                 |
| `%G`      | Shortest representation (`%F` or `%E`)                                                                                                 |
| `%%`      | Literal percent sign. No argument required.                                                                                            |

---

#### Combined examples

```php
sprintf("%8.2f", 3.1);    // "    3.10"
sprintf("%08.2f", 3.1);   // "00003.10"
sprintf("%6s", "lx");     // "    lx"
sprintf("%.3s", "lynx");  // "lyn"
sprintf("%04x", 15);      // "000f"
```

---

#### Notes and limitations

- Width and precision must be numeric literals.
- Dynamic width or precision using `*` is not supported.
- Unsupported or invalid format sequences are left unchanged in the output.