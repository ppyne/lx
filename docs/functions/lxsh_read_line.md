# lxsh_read_line

Read a line from the LX shell

Domain: Extensions: lxshcli

---

### Description

`lxsh_read_line([prompt]) : string`

Alias on LX shell builds: `read_line`.

Prints an optional prompt in the LX shell, reads a line from the
keyboard queue, and returns it without the trailing newline.

### Parameters

- **`prompt`**: Optional prompt to display before reading.

### Return Values

Returns the input line, or `undefined` on interrupt or error.

### Examples

```lx
$name = lxsh_read_line("Name: ");
print("Hello " . $name . "\n");
```
