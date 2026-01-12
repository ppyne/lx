# session_regenerate_id

Generate a new session ID

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`session_regenerate_id([delete_old]) : bool`

Generates a new session ID. When `delete_old` is `true`, the old session file
is removed.

### Parameters

- **`delete_old`**: Whether to delete the previous session file.

### Return Values

Returns `true` on success, `false` otherwise.
