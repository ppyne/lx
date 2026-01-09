# print_r

Print human-readable information

Domain: Output and formatting

---

### Description

`print_r(value[, return]) : string`

Prints a human-readable representation of `value`.
If `return` is `true`, the output is returned instead of printed.

### Parameters

- **`value`**: The value to print.
- **`return`** (optional): When `true`, returns the output as a string.

### Return Values

Returns the output string when `return` is `true`.

### Examples

```php
$a = [1, "a"];
print_r($a);
print(print_r($a, true));

/* Will output:
Array
(
    [0] => 1
    [1] => a
)
Array
(
    [0] => 1
    [1] => a
)
*/
```
