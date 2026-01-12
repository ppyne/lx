# tempnam

Create a temporary file and return its path

Domain: Extensions: fs

---

### Description

`tempnam([prefix]) : string|undefined`

Creates an empty temporary file and returns its path. The filename starts
with `prefix` when provided.

### Parameters

- **`prefix`** (optional): Filename prefix.

### Return Values

Returns the file path, or `undefined` on failure.

### Examples

```php
$tmp = tempnam("lx_");
print($tmp . "\n");
unlink($tmp);
```
