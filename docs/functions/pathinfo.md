# pathinfo

Path components

Domain: Extensions: fs

---

### Description

`pathinfo(path) : array`

Returns an array with path components.
The returned array has keys: dirname, basename, extension, filename.

### Parameters

- **`path`**: The path to analyze.

### Return Values

Returns an array.

### Examples

```php
var_dump(pathinfo("/a/b.txt"));

/* Will output:
array(4) {
  ["dirname"]=>
  string(2) "/a"
  ["basename"]=>
  string(5) "b.txt"
  ["extension"]=>
  string(3) "txt"
  ["filename"]=>
  string(1) "b"
}
*/
```
