# strrpos

Find last substring position

Domain: Strings

---

### Description

`strrpos(haystack, needle) : int|undefined`

Returns the last position of `needle` in `haystack`.

### Parameters

- **`haystack`**: The string to search in.
- **`needle`**: The string to search for.

### Return Values

Returns the zero-based position, or `undefined` if not found.

### Examples

```php
print(strrpos("hello", "l") . "\n");

/* Will output:
3
*/
```
