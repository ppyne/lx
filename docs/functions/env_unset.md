# env_unset

Unset an environment variable

Domain: Extensions: env

---

### Description

`env_unset(name) : bool`

Removes an environment variable by name.

### Parameters

- **`name`**: Environment variable name.

### Return Values

Returns `true` on success, `false` on failure.

### Examples

```php
print(env_set("LX_TEST_ENV", "abc") . "\n");
print(env_unset("LX_TEST_ENV") . "\n");
print(is_undefined(env_get("LX_TEST_ENV")) . "\n");

/* Will output:
true
true
true
*/
```
