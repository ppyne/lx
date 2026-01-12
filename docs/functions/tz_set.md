# tz_set

Set the default timezone

Domain: Extensions: time

---

### Description

`tz_set(name) : bool`

Sets the process timezone to `name` and calls `tzset()`.

### Parameters

- **`name`**: Timezone name (for example, `"UTC"` or `"Europe/Paris"`).

### Return Values

Returns `true` on success.
