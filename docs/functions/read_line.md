# read_line

Read a line from stdin

Domain: Extensions: cli

---

### Description

`read_line([prompt]) : string`

Prints an optional prompt, reads a line from standard input, and returns it
without the trailing newline.

### Parameters

- **`prompt`**: Optional prompt to display before reading.

### Return Values

Returns the input line, or `undefined` on EOF or error.

### Examples

```lx
$name = read_line("Name: ");
print("Hello " . $name . "\n");
```
