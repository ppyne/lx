# date

Format a local date/time

Domain: Extensions: time

---

### Description

`date(format[, timestamp]) : string`

Formats a local date/time according to `format`.

Supported format characters: `Y`, `y`, `m`, `n`, `d`, `j`, `S`, `H`, `h`, `G`, `i`, `s`, `a`, `A`, `M`, `F`, `D`, `l`, `w`, `z`, `W`, `L`, `t`, `U`, `c`, `r`. Use `\` to escape a character.
The `S` suffix is typically used together with `j`.

### Supported formats

| Character | Description | Example |
| --- | --- | --- |
| `Y` | Four-digit year | `2024` |
| `y` | Two-digit year | `24` |
| `m` | Month with leading zeros | `01` to `12` |
| `n` | Month without leading zeros | `1` to `12` |
| `d` | Day of the month with leading zeros | `01` to `31` |
| `j` | Day of the month without leading zeros | `1` to `31` |
| `S` | English ordinal suffix for the day of the month | `st`, `nd`, `rd`, `th` |
| `H` | 24-hour format with leading zeros | `00` to `23` |
| `h` | 12-hour format with leading zeros | `01` to `12` |
| `G` | 24-hour format without leading zeros | `0` to `23` |
| `i` | Minutes with leading zeros | `00` to `59` |
| `s` | Seconds with leading zeros | `00` to `59` |
| `a` | Lowercase ante/post meridiem | `am`, `pm` |
| `A` | Uppercase ante/post meridiem | `AM`, `PM` |
| `M` | Short textual month | `Jan` to `Dec` |
| `F` | Full textual month | `January` to `December` |
| `D` | Short textual day of the week | `Mon` to `Sun` |
| `l` | Full textual day of the week | `Monday` to `Sunday` |
| `w` | Numeric day of the week (0=Sunday) | `0` to `6` |
| `z` | Day of the year (starting from 0) | `0` to `365` |
| `W` | ISO 8601 week number of year, weeks starting on Monday | `01` to `53` |
| `L` | Whether it's a leap year | `1` if it is a leap year, `0` otherwise |
| `t` | Number of days in the month | `28` to `31` |
| `U` | Seconds since Unix Epoch | `1700000000` |
| `c` | ISO 8601 date (non-expanded) | `1970-01-01T00:00:00+00:00` |
| `r` | RFC 2822/RFC 5322 formatted date | `Thu, 01 Jan 1970 00:00:00 +0000` |

### Parameters

- **`format`**: The format string.
- **`timestamp`** (optional): Unix timestamp in seconds. Defaults to the current time.

### Return Values

Returns the formatted date/time string.

### Examples

```php
print(date("l jS \of F Y h:i:s A", 0), LX_EOL);

/* Will output:
Thursday 1st of January 1970 01:00:00 AM
*/
```
