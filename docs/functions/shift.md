# shift

Shift array start

Domain: Arrays

---

### Description

`shift(array) : mixed|undefined`

Removes and returns the first element of `array`.

### Parameters

- **`array`**: The array to operate on.

### Return Values

Returns the removed value, or `undefined` if the array is empty.

### Examples

```php
$a = [1, 2, 3];
var_dump(shift($a));

/* Will output:
int(1)
*/
```
