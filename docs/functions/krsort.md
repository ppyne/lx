# krsort

Sort array keys (descending)

Domain: Arrays

---

### Description

`krsort(array) : bool`

Sorts the keys of `array` in descending order while preserving key/value pairs.
Integer keys are compared numerically; string keys are compared lexicographically.

### Parameters

- **`array`**: The array to sort.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$a = ["b" => 2, "a" => 1, "c" => 3];
krsort($a);
print(join(keys($a), ",") . "\n");
print(join(values($a), ",") . "\n");

/* Will output:
c,b,a
3,2,1
*/

$fruits = array("d"=>"lemon", "a"=>"orange", "b"=>"banana", "c"=>"apple");
krsort($fruits);
foreach ($fruits as $key => $val) {
    echo "$key = $val\n";
}

/* Will output:
d = lemon
c = apple
b = banana
a = orange
*/
```
