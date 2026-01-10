# glyph_count

Count UTF-8 glyphs

Domain: Extensions: utf8

---

### Description

`glyph_count(string) : int|undefined`

Counts the number of UTF-8 glyphs (codepoints) in `string`.
Returns `undefined` if the input is not valid UTF-8.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the number of glyphs, or `undefined` on invalid UTF-8.

### Examples

```php
print(glyph_count("Aé💡") . "\n");

/* Will output:
3
*/
```
