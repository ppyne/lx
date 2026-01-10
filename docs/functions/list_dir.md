# list_dir

List directory entries

Domain: Extensions: fs

---

### Description

`list_dir(path) : array`

Lists directory entries (excluding . and ..), sorted.
Entries are returned as strings in sorted order.

### Parameters

- **`path`**: The directory path.

### Return Values

Returns an array.

### Examples

```php
$path = "./";
$files = list_dir($path);
foreach($files as $file) {
    if (is_dir($path . $file)) print("directory: " . $file . LX_EOL);
    else print("file: " . $file . LX_EOL);
}

/* Could output:
file: LICENSE
file: README.md
directory: docs
...
*/
```
