# write_blob

Write a blob to the CGI response body

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`write_blob(blob) : int`

Writes raw bytes from a blob into the HTTP response body.
Useful for returning binary data (images, PDFs, etc.).

### Parameters

- **`blob`**: Blob to write.

### Return Values

Returns the number of bytes written, or `undefined` on error.

### Examples

```lx
<?lx
header("Content-Type: image/png");
$data = file_get_contents("logo.png", true);
write_blob($data);
?>
```

See [Lx CGI wrapper](../lx_cgi.md) for usage details.
