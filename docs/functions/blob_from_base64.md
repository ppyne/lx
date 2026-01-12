# blob_from_base64

Decode base64 into a blob

Domain: Binary

---

### Description

`blob_from_base64(string) : blob`

Decodes a base64 string into a blob.

### Parameters

- **`string`**: Base64 input.

### Return Values

Returns a blob, or `undefined` on error.

### Examples

```php
$b = blob_from_base64("QUJD");
print(blob_size($b) . "\n");
```
