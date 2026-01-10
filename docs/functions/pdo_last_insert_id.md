# pdo_last_insert_id

Last inserted row id

Domain: Extensions: sqlite

---

### Description

`pdo_last_insert_id(db) : int`

Returns the last inserted row id for the database handle.

### Parameters

- **`db`**: Database handle.

### Return Values

Returns an integer id.

### Examples

```php
pdo_query($db, "INSERT INTO t (name) VALUES ('a')");
print(pdo_last_insert_id($db) . "\n");
```
