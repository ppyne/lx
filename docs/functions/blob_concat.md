# blob_concat

Concatenate blobs

Domain: Binary

---

### Description

`blob_concat(a, b) : blob`

Concatenates two blobs. Strings are converted to blobs before concatenation.

### Parameters

- **`a`**: Blob or string.
- **`b`**: Blob or string.

### Return Values

Returns a blob, or `undefined` on error.

### Examples

```php
$b = blob_concat("A", blob("B"));
print(blob_len($b) . "\n");
```
