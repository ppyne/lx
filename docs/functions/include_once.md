# include_once

Include a file once

Domain: Includes

---

### Description

`include_once(path) : bool`

Includes a file only on the first call with a given path.
On failure, a runtime error is raised and `false` is returned.

### Parameters

- **`path`**: The file path to include.

### Return Values

Returns `true` or `false`.
