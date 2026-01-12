# setcookie

Send a Set-Cookie header

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`setcookie(name, value[, expires[, path[, domain[, secure[, httponly]]]]]) : bool`

Adds a `Set-Cookie` header to the response when running under `lx_cgi`.

### Parameters

- **`name`**: Cookie name (required).
- **`value`**: Cookie value (required).
- **`expires`**: Unix timestamp (UTC) for the expiration date.
- **`path`**: Cookie path.
- **`domain`**: Cookie domain.
- **`secure`**: When `true`, adds the `Secure` attribute.
- **`httponly`**: When `true`, adds the `HttpOnly` attribute.

### Return Values

Returns `true` when the header is added, `false` on invalid input.

### Examples

```lx
setcookie("session", "abc123", 1735689600, "/");
```

See [Lx CGI wrapper](../lx_cgi.md) for usage details.
