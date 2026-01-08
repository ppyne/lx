# unshift

Prepend to array

Domain: Arrays

---

### Description

`unshift(array, value) : int`

Prepends `value` to `array` and returns the new length.

### Parameters

- **`array`**: The array to operate on.
- **`value`**: The value to process.

### Return Values

Returns an integer.

### Examples

```php
$a = [2, 3];
unshift($a, 1);
var_dump($a);

/* Will output:
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
*/
```
