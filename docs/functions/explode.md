# explode

Split string by delimiter

Domain: Strings

---

### Description

`explode(delim, string) : array`

Alias of `split` in Lx.
In Lx, an empty delimiter returns the original string as a single element.

### Parameters

- **`delim`**: The delimiter string.
- **`string`**: The input string.

### Return Values

Returns an array.

### Examples

```php
var_dump(explode(",", "a,b"));

/* Will output:
array(2) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
}
*/
```
