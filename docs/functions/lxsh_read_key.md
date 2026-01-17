# lxsh_read_key

Read a single key from the LX shell

Domain: Extensions: lxshcli

---

### Description

`lxsh_read_key([prompt]) : int`

Alias on LX shell builds: `read_key`.

Prints an optional prompt in the LX shell, reads a single byte from the
keyboard queue, and returns its numeric code. Input is handled by the
LX shell runtime while the script is active.

### Parameters

- **`prompt`**: Optional prompt to display before reading.

### Return Values

Returns the byte code as an integer, or `undefined` on interrupt or error.

### Examples

```lx
$code = lxsh_read_key("Press any key: ");
print("code=" . $code . "\n");
```
