# values

Array values

Domain: Arrays

---

### Description

`values(array) : array`

Returns an array of values from `array` with a new numerical indexes.

### Parameters

- **`array`**: The array to operate on.

### Return Values

Returns an indexed array of values.

### Examples

```php
var_dump(values(["a" => 10, "b" => 20]));

/* Will output:
array(2) {
  [0]=>
  int(10)
  [1]=>
  int(20)
}
*/
```
