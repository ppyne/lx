# var_dump

Dump values

Domain: Output and formatting

---

### Description

`var_dump(...values[, return]) : string`

Dumps information about one or more values.
Lx also reports `undefined` and `void` values.
If the last argument is a boolean, it is treated as the `return` flag.

### Parameters

- **`...values`**: Values to dump.
- **`return`** (optional): When `true`, returns the output as a string instead of printing it.

### Return Values

Returns the output string when `return` is `true`.

### Examples

```php
var_dump(1, "a", [1, 2]);
print(var_dump("x", true));

/* Will output:
int(1)
string(1) "a"
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
string(1) "x"
*/
```
