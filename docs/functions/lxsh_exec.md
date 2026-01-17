# lxsh_exec

Execute an LX shell command

Domain: Extensions: lxshexec

---

### Description

`lxsh_exec(command[, output]) : int`

Executes `command` via the LX shell and returns `0` on success or `1` on
failure. When `output` is provided and is an array, it is filled with rows of
`[line, stream_id]` using `LX_STDOUT` for `stream_id`. Pressing `Ctrl+C` while
the command is running requests cancellation.

### Parameters

- **`command`**: Command line to execute.
- **`output`**: Array to clear (optional).

### Return Values

Returns `0` on success or `1` on failure.

### Examples

```lx
$status = lxsh_exec("ls");
print($status . "\n");
```
