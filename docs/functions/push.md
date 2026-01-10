# push

Push onto array

Domain: Arrays

---

### Description

`push(array, value) : int`

Appends `value` to `array` and returns the new length.
This is equivalent to using `$array[] = value`.

### Parameters

- **`array`**: The array to operate on.
- **`value`**: The value to process.

### Return Values

Returns an integer.

### Examples

```php
$a = [1, 2];
push($a, 3);
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
