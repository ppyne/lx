# merge

Merge arrays

Domain: Arrays

---

### Description

`merge(array, array) : array`

Merges two arrays into a new array.
String keys overwrite earlier values; numeric keys are appended and reindexed.

### Parameters

- **`array_a`**: The first array to merge.
- **`array_b`**: The second array to merge.

### Return Values

Returns an array.

### Examples

```php
var_dump(merge([1, 2], [3, 4]));

/* Will output:
array(4) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
}
*/
```
