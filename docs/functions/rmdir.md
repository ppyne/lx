# rmdir

Remove directory

Domain: Extensions: fs

---

### Description

`rmdir(path) : bool`

Removes an empty directory.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns `true` or `false`.

### Examples

```php
if (!is_dir('example')) {
    mkdir('example');
}

rmdir('example');
```
