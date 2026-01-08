# starts_with

String prefix check

Domain: Strings

---

### Description

`starts_with(haystack, needle) : bool`

Checks whether `haystack` starts with `needle`.
An empty `needle` always returns true.

### Parameters

- **`haystack`**: The string to search in.
- **`needle`**: The string to search for.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(starts_with("hello", "he") . "\n");

/* Will output:
true
*/
```
