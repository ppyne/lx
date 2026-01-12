# blob_to_base64

Encode a blob to base64

Domain: Binary

---

### Description

`blob_to_base64(blob) : string`

Encodes a blob to a base64 string.

### Parameters

- **`blob`**: The input blob.

### Return Values

Returns a base64 string.

### Examples

```php
$b = blob("ABC");
print(blob_to_base64($b) . "\n"); // QUJD
```
