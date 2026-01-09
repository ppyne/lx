# asort

Sort array values and preserve keys

Domain: Arrays

---

### Description

`asort(array) : bool`

Sorts the values of `array` in ascending order while preserving keys.
Values are compared numerically when both are numbers; otherwise string comparison is used.

### Parameters

- **`array`**: The array to sort.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$a = ["b" => 2, "a" => 1, "c" => 3];
asort($a);
print(join(keys($a), ",") . "\n");
print(join(values($a), ",") . "\n");

/* Will output:
a,b,c
1,2,3
*/

$fruits = ["d" => "lemon", "a" => "orange", "b" => "banana", "c" => "apple"];
asort($fruits);
foreach ($fruits as $key => $val) {
    print($key, " = ", $val, "\n");
}

/* Will output:
c = apple
b = banana
d = lemon
a = orange
*/
```
