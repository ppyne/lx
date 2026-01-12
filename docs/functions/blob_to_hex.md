# blob_to_hex

Encode a blob to hex

Domain: Extensions: hex

---

### Description

`blob_to_hex(blob) : string`

Encodes a blob to a hex string.

### Parameters

- **`blob`**: The input blob.

### Return Values

Returns a hex string.

### Examples

```php
$b = blob("Hi");
print(blob_to_hex($b) . "\n"); // 4869
```
