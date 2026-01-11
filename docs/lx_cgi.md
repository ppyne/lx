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
- `$_POST` — parsed from POST body (form-urlencoded)
- `$_REQUEST` — merge of `$_GET` and `$_POST` (POST overrides)
- `$_SERVER` — all environment variables

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

- Only `application/x-www-form-urlencoded` POST bodies are parsed.
- File uploads and multipart are not supported.
- SQLite/FS/etc extensions are available (same as the normal `lx` binary).
