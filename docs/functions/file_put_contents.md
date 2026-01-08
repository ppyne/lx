# file_put_contents

Write file contents

Domain: Extensions: fs

---

### Description

`file_put_contents(path, data) : int`

Writes `data` to a file and returns the number of bytes written.

### Parameters

- **`path`**: The filesystem path.
- **`data`**: The data to write.

### Return Values

Returns an integer.

### Examples

```php
$file = 'fruits.txt';
// Open the file to get existing content
$current = file_get_contents($file);
// Append a new fruit to the file
$current .= "Banana\n";
// Write the contents back to the file
file_put_contents($file, $current);
```
