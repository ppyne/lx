# substr

Substring

Domain: Strings

---

### Description

`substr(string, start[, length]) : string`

Returns a substring of `string`.
In Lx, negative `start` values return an empty string.
If `length` is 0 or negative, the result is an empty string.

### Parameters

- **`string`**: The input string.
- **`start`**: The starting index (0-based).
- **`length`** (optional): The number of elements or characters to take.

### Return Values

Returns a string. The input value is converted to a string; use `blob_slice`
to slice blobs.

### Examples

```php
print(substr("abcd", 1, 2) . "\n");

/* Will output:
bc
*/
```
