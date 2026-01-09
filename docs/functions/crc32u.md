# crc32u

CRC32 checksum (unsigned)

Domain: Strings

---

### Description

`crc32u(string) : string`

Computes the CRC32 checksum of `string` and returns it as an unsigned decimal string.
This avoids negative values on 32-bit platforms.

### Parameters

- **`string`**: The input string.

### Return Values

Returns an unsigned decimal string checksum.

### Examples

```php
print(crc32u("123456789") . "\n");

/* Will output:
3421780262
*/
```
