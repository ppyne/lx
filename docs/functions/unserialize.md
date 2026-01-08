# unserialize

Unserialize value

Domain: Extensions: serializer

---

### Description

`unserialize(string) : mixed|undefined`

Parses a serialized string into a value.
Only scalar types and arrays are supported in Lx.

### Parameters

- **`string`**: The input string.

### Return Values

Returns the decoded value, or `undefined` on invalid input.

### Examples

```php
var_dump(unserialize("a:0:{}"));

/* Will output:
array(0) {
}
*/
```
