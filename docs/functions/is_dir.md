# is_dir

Check directory path

Domain: Extensions: fs

---

### Description

`is_dir(path) : bool`

Checks whether `path` is a directory.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns `true` or `false`.

### Examples

```php
var_dump(is_dir("a_file.txt"));
var_dump(is_dir("..")); //one dir up
```
