# header

Send a custom HTTP header

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`header(value) : void`

Sends a custom HTTP header when running under `lx_cgi`.
Use it to override the default Content-Type or add additional headers.

### Parameters

- **`value`**: Header line to send (for example: `"Content-Type: text/plain"`).

### Return Values

Returns `void`.

### Examples

```lx
<?lx
header("Content-Type: text/plain; charset=utf-8");
header("X-Powered-By: Lx");
print("Hello\n");
?>
```

See [Lx CGI wrapper](../lx_cgi.md) for usage details.
