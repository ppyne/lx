# usleep

Delay execution in microseconds

Domain: Extensions: time

---

### Description

`usleep(microseconds)`

Delays program execution for `microseconds`.

### Parameters

- **`microseconds`**: Number of microseconds to sleep.

### Return Values

This function does not return a value.

### Examples

```php
var_dump(is_void(usleep(0)));

/* Will output:
bool(true)
*/
```
