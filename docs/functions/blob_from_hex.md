# blob_from_hex

Decode hex into a blob

Domain: Extensions: hex

---

### Description

`blob_from_hex(string) : blob`

Decodes a hex string into a blob.

### Parameters

- **`string`**: Hex input.

### Return Values

Returns a blob, or `undefined` on error.

### Examples

```php
$b = blob_from_hex("4869");
print(blob_size($b) . "\n");
```
