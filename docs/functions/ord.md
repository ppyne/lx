# ord

Character code

Domain: Strings

---

### Description

`ord(string) : byte`

Returns the character code of the first byte in `string`.
If `string` is empty, the result is 0.

### Parameters

- **`string`**: The input string.

### Return Values

Returns a byte.

### Examples

```php
print(ord("A") . "\n");

/* Will output:
65
*/
```
