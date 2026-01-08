# var_dump

Dump values

Domain: Output and formatting

---

### Description

`var_dump(...values)`

Dumps information about one or more values.
Lx also reports `undefined` and `void` values.

### Parameters

- **`...values`**: Values to dump.

### Return Values

No return value.

### Examples

```php
var_dump(1, "a", [1, 2]);

/* Will output:
int(1)
string(1) "a"
array(2) {
  [0]=>
  int(1)
  [1]=>
  int(2)
}
*/
```
