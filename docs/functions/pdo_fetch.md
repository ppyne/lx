# pdo_fetch

Fetch the next row

Domain: Extensions: sqlite

---

### Description

`pdo_fetch(stmt) : array|undefined`

Fetches the next row from a prepared statement.
Returns `undefined` when there are no more rows.

### Parameters

- **`stmt`**: Statement handle.

### Return Values

Returns the next row as an array, or `undefined`.

### Examples

```php
pdo_execute($stmt, ["color" => "red"]);
$row = pdo_fetch($stmt);
print($row["name"] . "\n");
```

### PDO Guide

See [PDO/SQLite Guide](../lx_database_reference.md) for an introduction and examples.
