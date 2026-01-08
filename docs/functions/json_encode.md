# json_encode

Encode as JSON

Domain: Extensions: json

---

### Description

`json_encode(value) : string`

Encodes a value as a JSON string.
Arrays with string keys are encoded as objects; numeric keys are encoded as arrays.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns a string.

### Examples

```php
print(json_encode(["a" => 1, "b" => 2.1, "c" => "banana"]) . LX_EOL);

/* Will output:
{"a":1,"b":2.1,"c":"banana"}
*/
```
