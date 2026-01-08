# clamp

Clamp a value

Domain: Numeric and math

---

### Description

`clamp(value, min, max) : int|float`

Clamps `value` to the inclusive range [`min`, `max`].

### Parameters

- **`value`**: The value to process.
- **`min`**: The minimum bound.
- **`max`**: The maximum bound.

### Return Values

Returns an int when all arguments are integers, otherwise a float.

### Examples

```php
print(clamp(10, 0, 5) . "\n");

/* Will output:
5
*/
```
