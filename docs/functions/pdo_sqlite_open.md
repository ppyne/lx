# pdo_sqlite_open

Open a SQLite database

Domain: Extensions: sqlite

---

### Description

`pdo_sqlite_open(path) : resource|undefined`

Opens (or creates) a SQLite database and returns a database handle.
Returns `undefined` on failure.

### Parameters

- **`path`**: Path to the SQLite database file.

### Return Values

Returns a database handle, or `undefined` on error.

### Examples

```php
$db = pdo_sqlite_open("test.db");
print($db . "\n");
```

### PDO Guide

See [PDO/SQLite Guide](../lx_database_reference.md) for an introduction and examples.
