# join

Join array elements

Domain: Strings

---

### Description

`join(array, sep) : string`
`join(sep, array) : string`

Alias of `implode` in Lx.

### Parameters

- **`array`**: The array to operate on.
- **`sep`**: The separator string.

### Return Values

Returns a string.

### Examples

```php
print(join(";", ["a", "b"]) . "\n");

/* Will output:
a;b
*/
```
