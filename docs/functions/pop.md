# pop

Pop array end

Domain: Arrays

---

### Description

`pop(array) : mixed|undefined`

Removes and returns the last element of `array`.

### Parameters

- **`array`**: The array to operate on.

### Return Values

Returns the removed value, or `undefined` if the array is empty.

### Examples

```php
$a = [1, 2, 3];
var_dump(pop($a));
var_dump($a);

/* Will output:
int(3)
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
*/
```
