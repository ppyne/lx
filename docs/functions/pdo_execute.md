# pdo_execute

Execute a prepared statement

Domain: Extensions: sqlite

---

### Description

`pdo_execute(stmt[, params]) : bool`

Executes a prepared statement. Parameters may be passed as an array where keys
match the named placeholders (with or without the leading `:`).

### Parameters

- **`stmt`**: Statement handle.
- **`params`** (optional): Array of bound values.

### Return Values

Returns `true` on success, `false` on error.

### Examples

```php
$stmt = pdo_prepare($db, "SELECT name FROM fruit WHERE color = :color");
pdo_execute($stmt, ["color" => "red"]);
```
