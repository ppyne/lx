# gmdate

Format a UTC date/time

Domain: Extensions: time

---

### Description

`gmdate(format[, timestamp]) : string`

Formats a UTC date/time according to `format`.

Supported format characters: `Y`, `y`, `m`, `n`, `d`, `j`, `S`, `H`, `h`, `G`, `i`, `s`, `a`, `A`, `M`, `F`, `D`, `l`, `w`, `z`, `W`, `L`, `t`, `U`, `c`, `r`. Use `\` to escape a character.

### Parameters

- **`format`**: The format string.
- **`timestamp`** (optional): Unix timestamp in seconds. Defaults to the current time.

### Return Values

Returns the formatted UTC date/time string.

### Examples

```php
print(gmdate("Y-m-d H:i:s", 0) . "\n");

/* Will output:
1970-01-01 00:00:00
*/
```
