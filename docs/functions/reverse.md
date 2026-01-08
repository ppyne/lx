# reverse

Reverse array order

Domain: Arrays

---

### Description

`reverse(array) : array`

Returns a new array with elements in reverse order.
String keys are preserved; numeric keys are reindexed.

### Parameters

- **`array`**: The array to operate on.

### Return Values

Returns an array.

### Examples

```php
var_dump(reverse([1, 2, 3]));

/* Will output:
array(3) {
  [0]=>
  int(3)
  [1]=>
  int(2)
  [2]=>
  int(1)
}
*/
```
