# file_exists

Check file existence

Domain: Extensions: fs

---

### Description

`file_exists(path) : bool`

Checks whether a path exists.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(file_exists("existing.txt") . "\n");

/* Will output:
true
*/
```
