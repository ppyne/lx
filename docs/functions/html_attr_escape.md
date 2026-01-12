# html_attr_escape

Escape a string for use inside HTML attribute values

Domain: Strings

---

### Description

`html_attr_escape(value) : string`

Replaces double quotes with `&quot;`. Use it when interpolating user input
into HTML attributes.

### Parameters

- **`value`**: Value to escape.

### Return Values

Returns an escaped string.

### Examples

```lx
print('<input value="' . html_attr_escape('"hello"') . '">');
```
