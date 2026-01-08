# is_file

Check file path

Domain: Extensions: fs

---

### Description

`is_file(path) : bool`

Checks whether `path` is a regular file.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(is_file("README.md") . "\n");

/* Will output:
true
*/
```
