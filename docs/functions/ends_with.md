# ends_with

String suffix check

Domain: Strings

---

### Description

`ends_with(haystack, needle) : bool`

Checks whether `haystack` ends with `needle`.
An empty `needle` always returns true.

### Parameters

- **`haystack`**: The string to search in.
- **`needle`**: The string to search for.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(ends_with("hello", "lo") . "\n");

/* Will output:
true
*/
```
