# is_void

Void check

Domain: Types and inspection

---

### Description

`is_void(value) : bool`

Checks whether `value` is void. If a function ends without `return`, it returns `void`.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns `true` or `false`.

### Examples

```php
function f() { }
print(is_void(f()) . "\n");

/* Will output:
true
*/
```
