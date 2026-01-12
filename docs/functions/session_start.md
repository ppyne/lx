# session_start

Start or resume a session

Domain: HTTP
Context: CGI (in lx_cgi not available elsewhere)

---

### Description

`session_start([name]) : bool`

Starts a file-based session and exposes `$_SESSION`. If a session cookie is
present, resumes it; otherwise creates a new session.

### Parameters

- **`name`**: Optional session cookie name (defaults to `SESSION_NAME`).

### Return Values

Returns `true` on success, `false` otherwise.

### Examples

```lx
session_start();
$_SESSION["count"] = ($_SESSION["count"] ?? 0) + 1;
print($_SESSION["count"] . "\n");
```
