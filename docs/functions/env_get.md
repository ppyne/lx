# env_get

Get an environment variable

Domain: Extensions: env

---

### Description

`env_get(name[, default]) : string|undefined`

Gets an environment variable by name.

### Parameters

- **`name`**: Environment variable name.
- **`default`** (optional): Default value returned if the variable does not exist.

### Return Values

Returns the variable value as a string. If it does not exist, returns `default` when provided, otherwise `undefined`.

### Examples

```php
print(is_undefined(env_get("LX_TEST_ENV")) . "\n");
print(env_get("LX_TEST_ENV", "fallback") . "\n");

/* Will output:
true
fallback
*/
```
