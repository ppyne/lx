# is_defined

Defined value check

Domain: Types and inspection

---

### Description

`is_defined(value) : bool`

Checks whether `value` is defined (not undefined).

### Parameters

- **`value`**: The value to process.

### Return Values

Returns `true` or `false`.

### Examples

```php
if (is_defined($x)) {
    print("Defined");
} else {
    print("Not defined");
}

/* Will output:
Not defined
*/
```
