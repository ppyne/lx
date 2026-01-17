# exec

Execute a shell command

Domain: Extensions: exec

---

### Description

`exec(command[, output]) : int`

Executes `command` via `/bin/sh -c` and returns the exit status. When `output`
is provided and is an array, it is filled with rows of `[line, stream_id]`.
`stream_id` is `LX_STDOUT` or `LX_STDERR`.
On LX shell builds, use `lxsh_exec()` instead.

### Parameters

- **`command`**: Command line to execute.
- **`output`**: Array to receive output lines (optional).

### Return Values

Returns the process exit status, or `-1` on failure.

### Examples

```lx
$out = [];
$code = exec("echo hello; echo oops 1>&2", $out);
print($code . "\n");
print_r($out);
```
