# time

Current Unix timestamp

Domain: Extensions: time

---

### Description

`time() : int`

Returns the current Unix timestamp (seconds since 1970-01-01 00:00:00 UTC).

This value is timezone-agnostic (always UTC).

### Parameters

This function takes no parameters.

### Return Values

Returns the current Unix timestamp as an int.

### Examples

```php
var_dump(is_int(time()));

/* Will output:
bool(true)
*/
```
