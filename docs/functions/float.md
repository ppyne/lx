# float

Cast to float

Domain: Casting helpers

---

### Description

`float(value) : float`

Converts `value` to a float.
String values support decimal; hex or binary strings are parsed as integers first.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns a float.

### Examples

```php
var_dump(float("3.14"));

/* Will output:
3.14
*/
```
