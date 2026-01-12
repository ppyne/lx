# move_uploaded_file

Move an uploaded file to a destination path

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`move_uploaded_file(tmp_name, destination) : bool`

Moves a file uploaded through `multipart/form-data` from its temporary path
to the final destination. Returns `true` on success.

The source path must be a valid upload temp path from `$_FILES`.

### Parameters

- **`tmp_name`**: Temporary upload path (from `$_FILES`).
- **`destination`**: Target path to move the file to.

### Return Values

Returns `true` if the file is moved, `false` otherwise.

### Examples

```lx
<?lx
if ($_FILES["avatar"]["error"] == 0) {
    $tmp = $_FILES["avatar"]["tmp_name"];
    $ok = move_uploaded_file($tmp, "uploads/avatar.bin");
    print("upload=" . $ok . "\n");
}
?>
```

See [Lx CGI wrapper](../lx_cgi.md) for usage details.
