# Examples

Small Lx scripts demonstrating typical use cases.

## http_curl.lx

Implements `http_get`, `http_post`, and `http_request` in Lx by calling
the system `curl` command via `exec()`. Returns `[status, headers, body]`
and accepts optional headers and timeout values.

