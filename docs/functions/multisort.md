# multisort

Sort multiple arrays at once

Domain: Arrays

---

### Description

`multisort(array1[, order1[, mode1]], array2[, order2[, mode2]], ...) : bool`

Sorts multiple arrays using the first array as the primary key, then the
second, and so on. Arrays must have the same length. Each array can specify
an optional order (`SORT_ASC` or `SORT_DESC`) and comparison mode
(`SORT_REGULAR`, `SORT_NUMERIC`, `SORT_STRING`).

All arrays are reindexed from `0` to `n-1`.

### Parameters

- **`arrayN`**: Array to sort.
- **`orderN`**: `SORT_ASC` (default) or `SORT_DESC`.
- **`modeN`**: `SORT_REGULAR` (default), `SORT_NUMERIC`, or `SORT_STRING`.

### Return Values

Returns `true` on success, `false` if arrays are incompatible.

### Examples

```php
$names = ["bob", "alice", "bob"];
$scores = [2, 3, 1];
multisort($names, SORT_ASC, SORT_STRING, $scores, SORT_DESC, SORT_NUMERIC);
print_r($names);
print_r($scores);
```

Sorting by a "column" (like ORDER BY):

```php
$fruits = [
    ["name" => "apple", "color" => "red"],
    ["name" => "banana", "color" => "yellow"],
    ["name" => "cherry", "color" => "red"],
];

$names = [];
foreach ($fruits as $row) {
    $names[] = $row["name"];
}

multisort($names, SORT_ASC, SORT_STRING, $fruits);
print_r($fruits);
```
