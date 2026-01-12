# first

Return the first element of an array

Domain: Arrays

---

### Description

`first(array) : mixed|array`

Returns the first element of the array (index `0`) when present. If the input
is not an array or is empty, returns an empty array.

### Parameters

- **`array`**: Array to read from.

### Return Values

Returns the first element, or an empty array if missing.

### Examples

```lx
$items = [10, 20, 30];
print(first($items) . "\n");
```
