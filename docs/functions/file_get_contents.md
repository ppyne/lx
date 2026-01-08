# file_get_contents

Read file contents

Domain: Extensions: fs

---

### Description

`file_get_contents(path) : string|undefined`

Reads a file and returns its contents.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns the file contents, or `undefined` on error.

### Examples

```php
$html = file_getcontents("page.html");
print($html);
```
