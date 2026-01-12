# shell_escape

Escape a string for shell commands

Domain: Extensions: exec

---

### Description

`shell_escape(text) : string`

Escapes backslashes, quotes, and newlines for safe inclusion inside a
double-quoted shell string.

### Parameters

- **`text`**: Input text to escape.

### Return Values

Returns the escaped string.

### Examples

```lx
$path = "my file.txt";
$cmd = "cat \"" . shell_escape($path) . "\"";
print($cmd . "\n");
```
