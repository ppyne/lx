# implode

Join array elements

Domain: Strings

---

### Description

`implode(array, sep) : string`
`implode(sep, array) : string`

Joins array elements using `sep` and returns the resulting string.

### Parameters

- **`array`**: The array to operate on.
- **`sep`**: The separator string.

### Return Values

Returns a string.

### Examples

```php
print(implode(["a", "b"], ",") . "\n");

/* Will output:
a,b
*/
```
