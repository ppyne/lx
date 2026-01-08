# split

Split string by delimiter

Domain: Strings

---

### Description

`split(delim, string) : array`

Splits `string` by `delim` and returns an array of parts.
In Lx, an empty delimiter returns the original string as a single element.
This is different from PHP str_split, which splits by length.

### Parameters

- **`delim`**: The delimiter string.
- **`string`**: The input string.

### Return Values

Returns an array.

### Examples

```php
var_dump(split(",", "a,b"));

/* Will output:
array(2) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
}
*/
```
