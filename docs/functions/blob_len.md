# blob_len

Blob length

Domain: Binary

---

### Description

`blob_len(blob) : int`

Returns the size of a blob in bytes.

### Parameters

- **`blob`**: The input blob.

### Return Values

Returns the blob length, or `undefined` on error.

### Examples

```php
$b = blob("Hi");
print(blob_len($b) . "\n");
```
