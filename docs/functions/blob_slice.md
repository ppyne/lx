# blob_slice

Slice a blob

Domain: Binary

---

### Description

`blob_slice(blob, start[, length]) : blob`

Returns a new blob containing a slice of `blob`.

### Parameters

- **`blob`**: The input blob.
- **`start`**: Start index (0-based).
- **`length`** (optional): Number of bytes to take.

### Return Values

Returns a blob slice, or an empty blob if the range is out of bounds.

### Examples

```php
$b = blob("Hello");
$out = blob_slice($b, 1, 3);
print(blob_to_base64($out) . "\n");
```
