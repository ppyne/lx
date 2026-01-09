# env_set

Set an environment variable

Domain: Extensions: env

---

### Description

`env_set(name, value) : bool`

Creates or updates an environment variable.

### Parameters

- **`name`**: Environment variable name.
- **`value`**: Value to set.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
print(env_set("LX_TEST_ENV", "abc") . "\n");
print(env_get("LX_TEST_ENV") . "\n");

/* Will output:
true
abc
*/
```
