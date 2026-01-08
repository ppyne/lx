# include

Include a file

Domain: Includes

---

### Description

`include(path) : bool`

Parses and executes another Lx file in the current environment.
On failure, a runtime error is raised and `false` is returned.

### Parameters

- **`path`**: The file path to include.

### Return Values

Returns `true` or `false`.

### Examples

```php
// File: vars.lx
$color = "green";
$fruit = "apple";

// File: main.lx
print("A " . $color . " " . $fruit . PHP_EOL); // A
include("vars.lx");
print("A " . $color . " " . $fruit . PHP_EOL); // A green apple
```
