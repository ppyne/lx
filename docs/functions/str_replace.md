# str_replace

Replace substring

Domain: Strings

---

### Description

`str_replace(needle, replacement, haystack) : string`

Replaces all occurrences of `needle` in `haystack`.
`needle` and `replacement` can be arrays. When `needle` is an array, each
needle is replaced in order. If `replacement` is also an array and shorter
than `needle`, missing replacements become an empty string.

### Parameters

- **`needle`**: The string or array of strings to search for.
- **`replacement`**: The replacement value or array to insert.
- **`haystack`**: The string to search in.

### Return Values

Returns a string. All arguments are converted to strings.

### Examples

```php
print(str_replace("a", "b", "aabb") . "\n");

$s = str_replace(["&", "<", ">"], ["&amp;", "&lt;", "&gt;"], "<b>&</b>");
print($s . "\n");

/* Will output:
bbbb
&lt;b&gt;&amp;&lt;/b&gt;
*/
```
