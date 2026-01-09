# mv

Rename or move a file

Domain: Extensions: fs

---

### Description

`mv(source, destination) : bool`

Alias of `rename`.
If `destination` is a basename without `/`, the file is renamed inside the same directory as `source`.

### Parameters

- **`source`**: Path to the source file.
- **`destination`**: New path for the file.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
file_put_contents("a.txt", "hi");
print(mv("a.txt", "b.txt") . "\n");

/* Will output:
true
*/
```
