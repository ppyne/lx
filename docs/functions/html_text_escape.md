# html_text_escape

Escape a string for use inside HTML text nodes

Domain: Strings

---

### Description

`html_text_escape(value) : string`

Replaces `&`, `<`, and `>` with their HTML entities. Use it when interpolating
user input into HTML text content.

### Parameters

- **`value`**: Value to escape.

### Return Values

Returns an escaped string.

### Examples

```lx
print('<p>' . html_text_escape('5 < 6 & 7 > 4') . '</p>');
```
