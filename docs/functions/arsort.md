# arsort

Sort array values (descending) and preserve keys

Domain: Arrays

---

### Description

`arsort(array) : bool`

Sorts the values of `array` in descending order while preserving keys.
Values are compared numerically when both are numbers; otherwise string comparison is used.

### Parameters

- **`array`**: The array to sort.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$a = ["b" => 2, "a" => 1, "c" => 3];
arsort($a);
print(join(keys($a), ",") . "\n");
print(join(values($a), ",") . "\n");

/* Will output:
c,b,a
3,2,1
*/

$fruits = ["d" => "lemon", "a" => "orange", "b" => "banana", "c" => "apple"];
arsort($fruits);
foreach ($fruits as $key => $val) {
    print($key, " = ", $val, "\n");
}

/* Will output:
a = orange
d = lemon
b = banana
c = apple
*/
```
