# mktime

Build a local timestamp

Domain: Extensions: time

---

### Description

`mktime(hour, min, sec, month, day, year) : int`

Returns a Unix timestamp for a local date/time.

### Parameters

- **`hour`**: Hours.
- **`min`**: Minutes.
- **`sec`**: Seconds.
- **`month`**: Month number (1-12).
- **`day`**: Day of month (1-31).
- **`year`**: Full year (e.g., 2024).

### Return Values

Returns the Unix timestamp as an int.

### Examples

```php
var_dump(is_int(mktime(0, 0, 0, 1, 1, 1970)));

/* Will output:
bool(true)
*/
```
