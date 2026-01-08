# strpos

Find substring position

Domain: Strings

---

### Description

`strpos(haystack, needle) : int|undefined`

Returns the position of `needle` in `haystack`.

### Parameters

- **`haystack`**: The string to search in.
- **`needle`**: The string to search for.

### Return Values

Returns the zero-based position, or `undefined` if not found.

### Examples

```php
print(strpos("hello", "e") . "\n");

/* Will output:
1
*/
```
