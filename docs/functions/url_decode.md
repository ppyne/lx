# url_decode

Decode a URL-encoded string

Domain: Strings

---

### Description

`url_decode(string) : string`

Decodes a URL-encoded string. `+` becomes a space and `%HH` sequences
are decoded back to their byte values.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the decoded string.

### Examples

```php
print(url_decode("a+b%26c%3D1") . "\n");

/* Will output:
a b&c=1
*/
```
