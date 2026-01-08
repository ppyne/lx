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
var_dump(json_decode("{\"a\":1}"));

/* Will output:
array(1) {
  ["a"]=>
  int(1)
}
*/
```
