# print

Output values

Domain: Output and formatting

---

### Description

`print(...values)`

Outputs the string representation of each value. `array` values print as `array`. `blob` values print as `blob`. `void` prints nothing. `null`, `undefined`, `true`, and `false` print as their literal names.

### Parameters

- **`...values`**: Values to output.

### Return Values

No return value.

### Examples

```php
print("Hello", " ", "world", LX_EOL);

/* Will output:
Hello world
*/
```
