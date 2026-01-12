# Lx CGI Wrapper

This wrapper executes `.lx` templates that contain HTML mixed with `<?lx ... ?>` blocks,
similar to PHP. It is intentionally minimal and runs Lx in-process.

## Build

```sh
make lx_cgi
```

## Template format

Text outside `<?lx ... ?>` is emitted as HTML. Code inside the tag is executed.
The closing `?>` is optional if the file ends in Lx code.

Example of `hello.lx`:

```html
<h1>Hello</h1>
<?lx
$name = "world";
print("<p>Hello $name</p>");
?>
```

If you place this script in your local Apache environment, in your `DocumentRoot` directory, in your browser you may run `hello.lx` at the URL `http://localhost/hello.lx` depending of your own configuration.

## CGI environment variables

The wrapper exposes PHP-like globals:

- `$_GET` — parsed from `QUERY_STRING`
- `$_POST` — parsed POST body parameters (form-urlencoded or multipart fields)
- `$_REQUEST` — merge of `$_GET` and `$_POST` (POST overrides)
- `$_SERVER` — all environment variables
- `$_FILES` — uploaded files metadata (multipart only)
- `$_COOKIES` — parsed cookies from the `Cookie` header

Example:

```php
print($_GET["name"] . "\n");
print_r($_SERVER);
print_r($_REQUEST);
```

## Apache configuration (example)

This variant routes `.lx` files to `lx_cgi` using `ScriptAliasMatch`.
It requires `mod_alias` and a CGI module (`mod_cgi` or `mod_cgid`).

```
ScriptAliasMatch ^/(.*\\.lx)$ /path/to/lx_cgi/$1
```

You have to configure `/path/to` as a directory. Where is an example:

```
<Directory "/path/to">
    Options +ExecCGI
    AllowOverride None
    Options None
    Require all granted
</Directory>
```

`lx_cgi` resolves the script path as `DOCUMENT_ROOT + PATH_INFO`.
If your server passes `/lx_cgi/<file>` in `SCRIPT_FILENAME`, `lx_cgi` also
derives the path from that pattern.

**Note**: errors from Lx are returned into Apache's error_log file.

## Custom headers

`lx_cgi` exposes a `header()` function (CGI only) to send custom HTTP headers.
It also provides `setcookie()` to emit `Set-Cookie` headers.
The default response header is:

```
Content-Type: text/html; charset=utf-8
```

You can override it or add additional headers:

```
<?lx
// Delivering plain UTF-8 text
header("Content-Type: text/plain; charset=utf-8");
// Advertise the Lx technology
header('X-Powered-By: Lx ' . LX_VERSION);
// Print a greeting
print("Hello world!\n");
?>
```

## Notes

- `multipart/form-data` is supported for form fields and file uploads.
- File uploads are stored in a temp directory and removed at the end of the request.
- SQLite/FS/etc extensions are available (same as the normal `lx` binary).

## File uploads

When the request is `multipart/form-data`, file uploads are exposed through `$_FILES`.
Each field contains `name`, `type`, `size`, `tmp_name`, and `error`. If a field
has multiple files, these values become arrays (mirroring PHP's shape).

Use `move_uploaded_file(tmp_name, destination)` to move a temporary file into
its final location. The function returns `true` on success.

Upload behavior is controlled by `config.h`:

- `FILE_UPLOADS` (0/1): enable or disable uploads
- `UPLOAD_TMP_DIR`: temp directory for uploads
- `MAX_FILE_UPLOADS`: max number of uploaded files per request
- `UPLOAD_MAX_FILESIZE`: max size per file in bytes
- `POST_MAX_SIZE`: max total POST body size in bytes

If `POST_MAX_SIZE` is exceeded, `$_POST` and `$_FILES` are empty.
