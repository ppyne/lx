# date_tz

Format a date/time in a specific timezone

Domain: Extensions: time

---

### Description

`date_tz(format[, timestamp], timezone) : string`

Formats a date/time according to `format` using `timezone` (for example,
`"UTC"` or `"Europe/Paris"`).

Supports the same format characters as `date()`.

### Parameters

- **`format`**: The format string.
- **`timestamp`** (optional): Unix timestamp in seconds. Defaults to the current time.
- **`timezone`**: Timezone name.

### Return Values

Returns the formatted date/time string.

### Examples

```php
print(date_tz("Y-m-d H:i", 0, "UTC") . "\n");
print(date_tz("Y-m-d H:i", 0, "Europe/Paris") . "\n");
```
