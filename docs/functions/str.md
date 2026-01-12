# str

Cast to string

Domain: Casting helpers

---

### Description

`str(value) : string`

Converts `value` to a string.
When `value` is a blob, conversion stops at the first `0x00`, or at the blob length
if there is no `0x00`.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns a string.

### Examples

```php
var_dump(str(123));

/* Will output:
123
*/
```
