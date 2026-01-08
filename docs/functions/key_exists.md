# key_exists

Check array key

Domain: Arrays

---

### Description

`key_exists(key, array) : bool`

Checks whether `array` has the given `key`. The `key` can be a `string` or an `int` index.

### Parameters

- **`key`**: The key to check.
- **`array`**: The array to search.

### Return Values

Returns `true` or `false`.

### Examples

```php
print(key_exists(0, [10, 20]) . "\n");

/* Will output:
true
*/

print(key_exists("c", ["a" => 10, "b" => 20]) . "\n");

/* Will output:
false
*/
```
