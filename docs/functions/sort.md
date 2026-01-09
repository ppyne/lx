# sort

Sort array values

Domain: Arrays

---

### Description

`sort(array) : bool`

Sorts the values of `array` in ascending order.
String keys are discarded and numeric keys are reindexed from 0.
Values are compared numerically when both are numbers; otherwise string comparison is used.

**Note**: This function assigns new keys (indexes) to the elements in array. It will remove any existing keys that may have been assigned, rather than just reordering the keys.

### Parameters

- **`array`**: The array to sort.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$a = [3, 1, 2];
sort($a);
print(join($a, ",") . "\n");

/* Will output:
1,2,3
*/

$fruits = ["lemon", "orange", "banana", "apple"];
sort($fruits);
foreach ($fruits as $key => $val) {
    print("fruits[" . $key . "] = " . $val . "\n");
}

/* Will output:
fruits[0] = apple
fruits[1] = banana
fruits[2] = lemon
fruits[3] = orange
*/
```
