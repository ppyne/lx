# Examples

Small Lx scripts demonstrating typical use cases.

## http_curl.lx

Implements `http_get`, `http_post`, and `http_request` in Lx by calling
the system `curl` command via `exec()`. Returns `[status, headers, body]`
and accepts optional headers and timeout values.

## sendmail.lx

Sends an email via the local `sendmail` utility using `exec()`. Builds a
simple RFC-822 message with optional extra headers and returns the exit
code.

## imagemagick_resize.lx

Calls the ImageMagick `convert` CLI to resize an image while preserving
aspect ratio, fitting within a bounding box.
