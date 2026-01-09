# rename

Rename or move a file

Domain: Extensions: fs

---

### Description

`rename(source, destination) : bool`

Renames or moves a file from `source` to `destination`.
If `destination` is a basename without `/`, the file is renamed inside the same directory as `source`.
Alias: `mv`.

### Parameters

- **`source`**: Path to the source file.
- **`destination`**: New path for the file.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
file_put_contents("a.txt", "hi");
print(rename("a.txt", "b.txt") . "\n");
print(file_exists("a.txt") . "\n");
print(file_exists("b.txt") . "\n");

/* Will output:
true
false
true
*/
```
