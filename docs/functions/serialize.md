# serialize

Serialize value

Domain: Extensions: serializer

---

### Description

`serialize(value) : string`

Serializes a value using PHP-compatible format.
Only scalar types and arrays are supported in Lx.

### Parameters

- **`value`**: The value to process.

### Return Values

Returns a string.

### Examples

```php
$str = serialize([1, 2]);
print($str . LX_EOL);
$a = unserialize($str);
var_dump($a);

/* Will output:
a:2:{i:0;i:1;i:1;i:2;}
*/
```
