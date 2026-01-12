# blob

Cast to blob

Domain: Types and casting

---

### Description

`blob(value) : blob`

Creates a blob from the given value:

- `string`: copies its bytes (up to the string length)
- `byte`: one-byte blob
- `int` / `float`: raw in-memory bytes (native endianness)

The constant `LX_ENDIANNESS` indicates the host endianness (0 = little, 1 = big).

### Parameters

- **`value`**: Value to convert.

### Return Values

Returns a blob.

### Examples

```php
$b = blob("ABC");
print(blob_len($b) . "\n"); // 3
```
