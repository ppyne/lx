# blob_size

Blob size

Domain: Binary

---

### Description

`blob_size(blob) : int`

Returns the size of a blob in bytes.

### Parameters

- **`blob`**: The input blob.

### Return Values

Returns the blob size, or `undefined` on error.

### Examples

```php
$b = blob("Hi");
print(blob_size($b) . "\n");
```
