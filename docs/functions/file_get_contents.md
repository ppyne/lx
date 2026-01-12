# file_get_contents

Read file contents

Domain: Extensions: fs

---

### Description

`file_get_contents(path[, is_blob]) : string|blob|undefined`

Reads a file and returns its contents. When `is_blob` is `true`, the return
value is a blob.
When `is_blob` is `false`, the returned string stops at the first `0x00`.

### Parameters

- **`path`**: The filesystem path.
- **`is_blob`** (optional): When `true`, return a blob.

### Return Values

Returns the file contents, or `undefined` on error.

### Examples

```php
$html = file_get_contents("page.html");
print($html);
```
