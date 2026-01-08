# type

Runtime type

Domain: Types and inspection

---

### Description

`type(value) : string`

Returns the runtime type name of `value`.
Possible results: undefined, void, null, bool, int, float, string, array.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns a string.

### Examples

```php
print(type(123) . "\n");

/* Will output:
int
*/
```
