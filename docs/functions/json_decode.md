# json_decode

Decode JSON string

Domain: Extensions: json

---

### Description

`json_decode(string) : mixed|undefined`

Decodes a JSON string into a value.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the decoded value, or `undefined` on invalid JSON.

### Examples

```php
$json = "{\"a\":1}";
if (is_json($json)) {
    print_r(json_decode($json));
}

/* Will output:
Array
(
    [a] => 1
)
*/
```
