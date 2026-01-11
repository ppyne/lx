# Lx Database reference – PDO (SQLite)

Lx provides a minimal PDO-like mechanism for SQLite via the `sqlite` extension.
It lets you open a database, run simple queries, and use prepared statements
with named parameters.

---

## Complete example: "fruit" database

### 1. Database creation

We start by creating a database named `fruits`.

```php
$db = pdo_sqlite_open("data/fruits.db");
```

**Note**: the database is created on disk if it does not exist, so you need proper permissions in the
directory where you create it.

### 2. Table creation

A `fruit` table with a few attributes is created.

```php
pdo_query($db, "CREATE TABLE IF NOT EXISTS fruit ("
    . "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    . "name TEXT, "
    . "color TEXT, "
    . "season TEXT, "
    . "sweetness INT"
    . ")");
```

### 3. Data insertion

We then insert several fruits and deliberately add a vegetable.

```php
pdo_query($db, "INSERT INTO fruit (name, color, season, sweetness) VALUES "
    . "('apple', 'red', 'fall', 7), "
    . "('banana', 'yellow', 'all_year', 6), "
    . "('strawberry', 'red', 'spring', 8)");

pdo_query($db, "INSERT INTO fruit (name, color, season, sweetness) VALUES "
    . "('carrot', 'orange', 'winter', 3)");

pdo_query($db, "INSERT INTO fruit (name, color) VALUES ('pineapple', 'yellow')");
```

### 4. Data update

We update data to fix an incomplete insertion using `pdo_last_insert_id()`.

```php
$last_id = pdo_last_insert_id($db);
pdo_query($db, "UPDATE fruit SET season = 'summer', sweetness = 7 "
    . "WHERE id = " . $last_id);
```

### 5. Data deletion

We remove the vegetable from the `fruit` table.

```php
pdo_query($db, "DELETE FROM fruit WHERE name = 'carrot'");
```

---

## Simple query with `pdo_query`

Now that the database is ready, we fetch a quick list of fruits to show the
basic, one-shot query API.

```php
$rows = pdo_query($db, "SELECT name, color FROM fruit ORDER BY name");
print_r($rows);
```

---

## Prepared query with parameters

Next, we reuse a prepared statement to pull red fruits first, then yellow ones,
to demonstrate named parameters and repeated execution.

```php
$stmt = pdo_prepare($db, "SELECT name, color FROM fruit WHERE color = :color");

pdo_execute($stmt, ["color" => "red"]);
$red = pdo_fetch_all($stmt);
print_r($red);

pdo_execute($stmt, ["color" => "yellow"]);
$yellow = pdo_fetch_all($stmt);
print_r($yellow);
```

This approach is efficient in a loop because you reuse the same prepared statement.

```php
$colors = ["red", "yellow", "orange"];
$stmt = pdo_prepare($db, "SELECT name FROM fruit WHERE color = :color");

foreach ($colors as $color) {
    pdo_execute($stmt, ["color" => $color]);
    $rows = pdo_fetch_all($stmt);
    print_r($rows);
}
```

---

## Useful functions

- [`pdo_sqlite_open`](functions/pdo_sqlite_open.md) to open a SQLite database.
- [`pdo_query`](functions/pdo_query.md) to execute a simple query.
- [`pdo_prepare`](functions/pdo_prepare.md) + [`pdo_execute`](functions/pdo_execute.md) for parameterized queries.
- [`pdo_fetch`](functions/pdo_fetch.md) / [`pdo_fetch_all`](functions/pdo_fetch_all.md) to read results.
- [`pdo_last_insert_id`](functions/pdo_last_insert_id.md) to get the last id.
- [`pdo_close`](functions/pdo_close.md) to close the database.
