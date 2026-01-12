# read_key

Read a single key from stdin

Domain: Extensions: cli

---

### Description

`read_key([prompt]) : int`

Prints an optional prompt, reads a single byte from standard input, and
returns its numeric code. When reading from a terminal, input is read in
raw mode without echo.

### Parameters

- **`prompt`**: Optional prompt to display before reading.

### Return Values

Returns the byte code as an integer, or `undefined` on EOF or error.

### Examples

```lx
$code = read_key("Press any key: ");
print("code=" . $code . "\n");
```
