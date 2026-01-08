# chr

Character from code

Domain: Strings

---

### Description

`chr(code) : string`

Returns the character for the given code (0-255).
`code` is clamped to the range 0..255.

### Parameters

- **`code`**: The character code.

### Return Values

Returns a string.

### Examples

```php
print(chr(65) . "\n");

/* Will output:
A
*/
```
