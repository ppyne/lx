# is_blob

Check if a value is a blob

Domain: Types and inspection

---

### Description

`is_blob(value) : bool`

Returns `true` if `value` is a blob, otherwise `false`.

### Parameters

- **`value`**: The value to test.

### Return Values

Returns a boolean.

### Examples

```php
$b = blob("ABC");
print(is_blob($b) . "\n"); // true
```
