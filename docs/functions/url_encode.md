# url_encode

Encode a string for use in a URL query

Domain: Strings

---

### Description

`url_encode(string) : string`

Encodes `string` using URL encoding. Spaces become `+`, and other bytes
are percent-encoded as `%HH`.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the URL-encoded string.

### Examples

```php
print(url_encode("a b&c=1") . "\n");

/* Will output:
a+b%26c%3D1
*/
```
