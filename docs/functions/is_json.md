# is_json

Validate JSON string

Domain: Extensions: json

---

### Description

`is_json(string) : bool`

Checks whether `string` contains valid JSON.

### Parameters

- **`string`**: The input string.

### Return Values

Returns `true` if the string is valid JSON, otherwise `false`.

### Examples

```php
print(is_json("{\"a\":1}") . "\n");
print(is_json("bad") . "\n");

/* Will output:
true
false
*/
```
