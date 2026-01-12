# str_replace

Replace substring

Domain: Strings

---

### Description

`str_replace(needle, replacement, haystack) : string`

Replaces all occurrences of `needle` in `haystack`.

### Parameters

- **`needle`**: The string to search for.
- **`replacement`**: The replacement value or array to insert.
- **`haystack`**: The string to search in.

### Return Values

Returns a string. All arguments are converted to strings.

### Examples

```php
print(str_replace("a", "b", "aabb") . "\n");

/* Will output:
bbbb
*/
```
