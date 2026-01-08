# unlink

Delete file

Domain: Extensions: fs

---

### Description

`unlink(path) : bool`

Deletes a file.

### Parameters

- **`path`**: The filesystem path.

### Return Values

Returns `true` or `false`.

### Examples

```php
$filename = "temp.txt";
file_put_contents($filename, "Temporary data");
if (file_exists($filename)) {
    print("Removing existing temporary file.\n");
    if (unlink($filename)) print ("File ", $filename, " removed succesfully.\n");
    else print("An unexpected problem prevented the file ", $filename, " from being deleted.\n");
}
/* Should output:
Removing existing temporary file.
File temp.txt removed succesfully.
*/
```
