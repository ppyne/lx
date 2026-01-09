# copy

Copy a file

Domain: Extensions: fs

---

### Description

`copy(source, destination) : bool`

Copies a file from `source` to `destination`.
Alias: `cp`.

### Parameters

- **`source`**: Path to the source file.
- **`destination`**: Path to the destination file.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
file_put_contents("a.txt", "hi");
print(copy("a.txt", "b.txt") . "\n");
print(file_get_contents("b.txt") . "\n");

/* Will output:
true
hi
*/
```
