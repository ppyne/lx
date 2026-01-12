# printf

Formatted output

Domain: Output and formatting

---

### Description

`printf(format, ...args)`

Outputs a formatted string using printf-style formatting.

`printf` behaves like `print(sprintf(format, ...args))`.  

For a detailed description of the formatting syntax and supported specifiers, see [**`sprintf`**](sprintf.md).

Note: `byte` arguments are only supported with `%c`, `%d`, `%u`, `%x`, and `%X`.

### Parameters

- **`format`**: The format string.
- **`...args`**: Values to format into the string.

### Return Values

No return value.

### Examples

```php
printf("Value: %03d\n", -7);

/* Will output:
Value: -07
*/
```
