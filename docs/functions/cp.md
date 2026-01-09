# cp

Copy a file

Domain: Extensions: fs

---

### Description

`cp(source, destination) : bool`

Alias of `copy`.

### Parameters

- **`source`**: Path to the source file.
- **`destination`**: Path to the destination file.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
file_put_contents("a.txt", "hi");
print(cp("a.txt", "b.txt") . "\n");

/* Will output:
true
*/
```
