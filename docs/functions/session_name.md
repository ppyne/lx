# session_name

Get or set the session cookie name

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`session_name([name]) : string`

Returns the current session cookie name. If `name` is provided, updates it.

### Parameters

- **`name`**: Optional session cookie name.

### Return Values

Returns the current session name.
