# slice

Array slice

Domain: Arrays

---

### Description

`slice(array, start[, length]) : array`

Returns a slice of `array` starting at `start`.
In Lx, negative `start` values are clamped to 0.

### Parameters

- **`array`**: The array to operate on.
- **`start`**: The starting index (0-based).
- **`length`** (optional): The number of elements or characters to take.

### Return Values

Returns an array.

### Examples

```php
var_dump(slice([1, 2, 3, 4], 1, 2));

/* Will output:
array(2) {
  [0]=>
  int(2)
  [1]=>
  int(3)
}
*/
```
