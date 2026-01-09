# env_list

List environment variables

Domain: Extensions: env

---

### Description

`env_list() : array`

Returns all environment variables as an associative array.

### Parameters

This function takes no parameters.

### Return Values

Returns an array where keys are variable names and values are strings.

### Examples

```php
env_set("LX_TEST_ENV", "abc");
$vars = env_list();
print($vars["LX_TEST_ENV"] . "\n");

/* Will output:
abc
*/
```
