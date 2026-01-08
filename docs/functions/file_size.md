# file_size

Get file size

Domain: Extensions: fs

---

### Description

`file_size(path) : int|undefined`

Returns the size of a file in bytes.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns the file size in bytes, or `undefined` on error.

### Examples

```php
$filename = 'somefile.txt';
print($filename . ": " . file_size(filename) . " bytes\n");

/* Could output:
somefile.txt: 1024 bytes
*/
```
