# sleep

Delay execution

Domain: Extensions: time

---

### Description

`sleep(seconds) : int`

Delays program execution for `seconds`.

### Parameters

- **`seconds`**: Number of seconds to sleep.

### Return Values

Returns the number of seconds left to sleep if interrupted, otherwise `0`.

### Examples

```php
var_dump(sleep(0));

/* Will output:
int(0)
*/
```
