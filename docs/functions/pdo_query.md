# pdo_query

Execute a SQL query

Domain: Extensions: sqlite

---

### Description

`pdo_query(db, sql) : array|undefined`

Executes `sql` and returns an array of rows for SELECT queries.
For non-SELECT queries, returns an empty array.

### Parameters

- **`db`**: Database handle.
- **`sql`**: SQL string.

### Return Values

Returns an array of rows, or `undefined` on error.

### Examples

```php
$rows = pdo_query($db, "SELECT name FROM fruit");
print($rows[0]["name"] . "\n");
```

### PDO Guide

See [PDO/SQLite Guide](../lx_database_reference.md) for an introduction and examples.
