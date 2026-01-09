# ksort

Sort array keys

Domain: Arrays

---

### Description

`ksort(array) : bool`

Sorts the keys of `array` in ascending order while preserving key/value pairs.
Integer keys are compared numerically; string keys are compared lexicographically.

### Parameters

- **`array`**: The array to sort.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$a = ["b" => 2, "a" => 1, "c" => 3];
ksort($a);
print(join(keys($a), ",") . "\n");
print(join(values($a), ",") . "\n");

/* Will output:
a,b,c
1,2,3
*/

$fruits = ["d"=>"lemon", "a"=>"orange", "b"=>"banana", "c"=>"apple"];
ksort($fruits);
foreach ($fruits as $key => $val) {
    print($key, " = ", $val, "\n");
}

/* Will output:
a = orange
b = banana
c = apple
d = lemon
*/
```
