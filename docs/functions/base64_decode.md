# base64_decode

Base64 decode a string

Domain: Strings

---

### Description

`base64_decode(string) : string|undefined`

Decodes a Base64-encoded `string`.

### Parameters

- **`string`**: The Base64-encoded string.

### Return Values

Returns the decoded string, or `undefined` if the input is invalid.

### Examples

```php
print(base64_decode("SGVsbG8=") . "\n");
print(base64_decode("###") . "\n");

/* Will output:
Hello
undefined
*/
```
