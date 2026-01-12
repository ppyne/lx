# session_destroy

Destroy the current session

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`session_destroy() : bool`

Deletes the current session file and expires the session cookie.

### Return Values

Returns `true` on success, `false` otherwise.
