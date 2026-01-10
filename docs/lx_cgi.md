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

Example:

```html
<h1>Hello</h1>
<?lx
$name = "world";
print("Hello " . $name);
?>
```

## CGI environment variables

The wrapper exposes PHP-like globals:

- `$_GET` — parsed from `QUERY_STRING`
- `$_POST` — parsed from POST body (form-urlencoded)
- `$_REQUEST` — merge of `$_GET` and `$_POST` (POST overrides)
- `$_SERVER` — all environment variables

Example:

```php
print($_GET["name"] . "\n");
```

## Apache configuration (simple CGI with mod_actions)

```
ScriptAlias /lx_cgi /path/to/lx_cgi
AddHandler lx-script .lx
Action lx-script /lx_cgi
```

## Apache configuration (without mod_actions)

This variant routes `.lx` files to `lx_cgi` using `ScriptAliasMatch`.
It requires `mod_alias` and a CGI module (`mod_cgi` or `mod_cgid`).

Recommended (sets `PATH_INFO`):

```
ScriptAliasMatch ^/(.*\\.lx)$ /path/to/lx_cgi/$1
```

If you already use the `$1` suffix form, `lx_cgi` will also try to resolve
`DOCUMENT_ROOT + /$1` when it sees `/lx_cgi/<file>` in `SCRIPT_FILENAME`.

## Custom headers

`lx_cgi` exposes a `header()` function (CGI only) to send custom HTTP headers.
The default response header is:

```
Content-Type: text/html; charset=utf-8
```

You can override it or add additional headers:

```lx
<?lx
header("Content-Type: text/plain; charset=utf-8");
header("X-Powered-By: Lx");
print("Hello\n");
?>
```

## Notes

- Only `application/x-www-form-urlencoded` POST bodies are parsed.
- File uploads and multipart are not supported.
- SQLite/FS/etc extensions are available (same as the normal `lx` binary).
