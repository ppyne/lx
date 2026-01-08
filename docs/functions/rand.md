# rand

Random integer

Domain: Numeric and math

---

### Description

`rand() : int`
`rand(max) : int`
`rand(min, max) : int`

Returns a random integer, optionally within a range.
With one argument, the range is 0..max (inclusive).
If `min` > `max`, the values are swapped.

### Parameters

- **`max`** (optional): The upper bound (inclusive).
- **`min`** (optional): The lower bound (inclusive).

### Return Values

Returns a random integer in the requested range (inclusive).

### Examples

```php
print(rand(1, 6) . "\n");

/* Could output:
3
*/
```
