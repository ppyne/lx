# splice

Remove and replace slice

Domain: Arrays

---

### Description

`splice(array, start[, length[, replacement]]) : array`

Removes a slice from `array` and optionally inserts `replacement`.
In Lx, negative `start` values are clamped to 0.

### Parameters

- **`array`**: The array to operate on.
- **`start`**: The start index (0-based).
- **`length`** (optional): The number of elements or characters to take.
- **`replacement`** (optional): The value or array to insert.

### Return Values

Returns an array.

### Examples

```php
$a = [1, 2, 3, 4];
var_dump(splice($a, 1, 2));
var_dump($a);

/* Will output:
array(2) {
  [0]=>
  int(2)
  [1]=>
  int(3)
}
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(4)
}
*/
```
