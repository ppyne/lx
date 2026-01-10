# pdo_fetch_all

Fetch all rows

Domain: Extensions: sqlite

---

### Description

`pdo_fetch_all(stmt) : array`

Fetches all remaining rows from a prepared statement.

### Parameters

- **`stmt`**: Statement handle.

### Return Values

Returns an array of rows.

### Examples

```php
pdo_execute($stmt, ["color" => "red"]);
$rows = pdo_fetch_all($stmt);
print($rows[0]["name"] . "\n");
```
