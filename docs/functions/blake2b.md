# blake2b

BLAKE2b hash

Domain: Extensions: blake2b

---

### Description

`blake2b(string[, out_len[, base64]]) : string|undefined`

Computes a BLAKE2b hash of `string`.
By default the output length is 4 bytes (32 bits) and the output is hex.
When `base64` is true, the output is Base64 instead of hex.
`out_len` is clamped to the range 1..64.

### Parameters

- **`string`**: The input string.
- **`out_len`** (optional): The output length in bytes (1..64).
- **`base64`** (optional): When true, return Base64 instead of hex.

### Return Values

Returns the hash string, or `undefined` on error.

### Examples

```php
print(blake2b("", 4) . "\n");
print(blake2b("", 8) . "\n");
print(blake2b("abc", 4, true) . "\n");

/* Will output:
1271cf25
e4a6a0577479b2b4
Y5BiSA==
*/
```
