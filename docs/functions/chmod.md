# chmod

Change file permissions

Domain: Extensions: fs

---

### Description

`chmod(path, mode) : bool`

Changes file permissions for `path` using the numeric `mode` value.

### Parameters

- **`path`**: The filesystem path.
- **`mode`**: The permission mode (numeric).

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
file_put_contents("tmp_file.txt", "abc");
print(chmod("tmp_file.txt", 0644) . "\n");
unlink("tmp_file.txt");

/* Will output:
true
*/
```
