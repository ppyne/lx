# crc32

CRC32 checksum (signed)

Domain: Strings

---

### Description

`crc32(string) : int`

Computes the CRC32 checksum of `string`.
This function returns a signed 32-bit integer, which can be negative.

### Parameters

- **`string`**: The input string.

### Return Values

Returns a signed 32-bit integer checksum.

### Examples

```php
print(crc32("123456789") . "\n");

/* Will output:
-873187034
*/
```
