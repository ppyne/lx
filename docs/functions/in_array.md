# in_array

Search for value in array

Domain: Arrays

---

### Description

`in_array(value, array) : bool`

Checks whether `array` contains `value` using Lx equality rules.

### Parameters

- **`value`**: The value to search for.
- **`array`**: The array to search.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(in_array(2, [1, 2, 3]) . "\n");

/* Will output:
true
*/
```
