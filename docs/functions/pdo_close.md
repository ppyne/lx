# pdo_close

Close a database handle

Domain: Extensions: sqlite

---

### Description

`pdo_close(db) : bool`

Closes the database handle and frees related prepared statements.

### Parameters

- **`db`**: Database handle.

### Return Values

Returns `true` on success, otherwise `false`.

### Examples

```php
$db = pdo_sqlite_open("test.db");
print(pdo_close($db) . "\n");
```
