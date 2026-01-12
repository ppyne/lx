# strlen

String length

Domain: Strings

---

### Description

`strlen(string) : int`

Returns the length of `string`.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the string length after converting the value to a string. Use
`blob_len` to get the full length of a blob.

### Examples

```php
print(strlen("abcd") . "\n");

/* Will output:
4
*/
```
