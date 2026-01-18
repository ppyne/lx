# print

Output values

Domain: Output and formatting

---

### Description

`print(...values)`

Outputs the string representation of each value. `array` values print as `array`. `blob` values print as `blob`. `void` prints nothing. `null`, `undefined`, `true`, and `false` print as their literal names.

Note: When running on a terminal, stdout is line-buffered. If you print a prompt without a newline, it may not appear until a newline is printed or input is read. Prefer `read_key("...")` for prompts, or include `\n` when you need immediate output.

### Parameters

- **`...values`**: Values to output.

### Return Values

No return value.

### Examples

```php
print("Hello", " ", "world", LX_EOL);

/* Will output:
Hello world
*/
```
