# in_array

Search for value in array

Domain: Arrays

---

### Description

`in_array(value, array[, strict]) : bool`

Checks whether `array` contains `value`. By default, comparisons are strict.
Set `strict` to `false` to use loose comparison.

### Parameters

- **`value`**: The value to search for.
- **`array`**: The array to search.
- **`strict`** (optional): If `true` (default), compare types strictly. If `false`, use loose comparison.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(in_array(2, [1, 2, 3]) . "\n");
print(in_array("2", [1, 2, 3]) . "\n");
print(in_array("2", [1, 2, 3], false) . "\n");

/* Will output:
true
false
true
*/
```
