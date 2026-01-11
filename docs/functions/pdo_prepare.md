# pdo_prepare

Prepare a SQL statement

Domain: Extensions: sqlite

---

### Description

`pdo_prepare(db, sql[, options]) : stmt|undefined`

Prepares `sql` and returns a statement handle.

### Parameters

- **`db`**: Database handle.
- **`sql`**: SQL string.
- **`options`** (optional): Options array (currently ignored).

### Return Values

Returns a statement handle, or `undefined` on error.

### Examples

```php
$stmt = pdo_prepare($db, "SELECT name FROM fruit WHERE color = :color");
```

### PDO Guide

See [PDO/SQLite Guide](../lx_database_reference.md) for an introduction and examples.
