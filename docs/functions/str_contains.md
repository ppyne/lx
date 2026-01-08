# str_contains

Substring check

Domain: Strings

---

### Description

`str_contains(haystack, needle) : bool`

Checks whether `haystack` contains `needle`.
An empty `needle` always returns true.

### Parameters

- **`haystack`**: The string to search in.
- **`needle`**: The string to search for.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(str_contains("hello", "ell") . "\n");

/* Will output:
true
*/
```
