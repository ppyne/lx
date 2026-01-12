# byte

Cast to byte

Domain: Types and casting

---

### Description

`byte(value) : byte`

Clamps `value` to the range 0..255 and returns a byte.

### Parameters

- **`value`**: Any numeric value.

### Return Values

Returns a byte (0..255).

### Examples

```php
print(byte(300) . "\n"); // 255
print(byte(-5) . "\n");  // 0
```
