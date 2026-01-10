# Lx Language — User Manual

<img src="lx_mascot.svg" width="128" alt="Lx mascot">

Lx: Power without noise

---

## 1. Introduction

### 1.1 What is Lx?

Lx is a general-purpose interpreted, dynamically typed, scripting language, with a PHP-like surface syntax.

Programs are parsed and executed directly by the interpreter.

Lx scripts are composed of statements terminated by semicolons. Blocks are delimited with `{ ... }`. Expressions follow a conventional operator precedence model.

This manual describes **only the behavior currently implemented**.

**Name origin and philosophy**

The name **Lx** originates from the author’s name, **Alex**, pronounced using only the letters **L** and **X**. 

It reflects a deliberate simplification: a short, minimal name aligned with the language’s emphasis on clarity, explicitness, and reduced complexity.

It deliberately aligns with the early PHP philosophy promoted by Rasmus Lerdorf, where simplicity, directness, and practical usefulness took precedence over formal or abstract language design.

**Mascot choice**

The Lx language mascot is a **lynx**.  
The lynx was chosen for its symbolism: sharp vision, precision, and quiet attentiveness. These traits reflect Lx’s design goals—clarity, explicit behavior, and the absence of hidden mechanisms.

The lynx also echoes the name **Lx** itself, reinforcing a strong and simple identity. Its minimalist representation aligns with the language’s focus on frugality and readability, even at very small scales such as icons.

**Installation and building**

See [Lx Installation and building](lx_installation.md) for details.

**Running**

```sh
./lx script.lx
cat script.lx | ./lx
echo "print(lxinfo());" | ./lx
./lx < script.lx
```

---

## 2. Language Basics

### 2.1 Program structure

An Lx program is a sequence of statements executed in order.

```php
$a = 1;
print($a);
```

Rules:

- Each statement must end with `;`.
- Blocks use `{` and `}`.
- Missing semicolons cause a parse error.

---

### 2.2 Comments

Lx supports line and block comments.

```php
# line comment
// line comment
/* block comment */

$a = 5 /* inline comment */ + 2;

#!/usr/local/bin/lx
```

Comments may appear anywhere whitespace is allowed.

---

### 2.3 Variables

Variables are prefixed with `$` and are created by assignment.

```php
$a = 10;
```

Assignments create or update a variable in the **current scope**.  
To read or modify globals from inside a function, use `global`.

Lx also supports **variable variables** using `$$`, like PHP.

```php
$a = 'b';
$b = 'c';
$c = $$a;
print($c . "\n"); // c

$$a = 'd';
print($b . "\n"); // d
```

**Precedence note:** `$$name["x"]` is conceptually parsed as `$( $name["x"] )`.  
To index the variable named by `$$name`, use parentheses:

```php
$name = "arr";
$arr = ["x" => 1];
print(($$name)["x"] . "\n"); // 1
```

---

## 3. Types and Values

Lx supports the following runtime types:

- `int`: signed integer
- `float`: double-precision floating-point number
- `bool`: boolean value (true or false)
- `string`: character string
- `array`: associative array with integer or string keys
- `null`: explicit absence of value
- `undefined`: value of an uninitialized or missing variable
- `void`: default silent function return value

Type inspection is available via `type()` and `is_*()` functions.

```php
$i = 42;
print(type($i) . "\n"); // Prints int

$str = 'Hello world';
print(type($str) . LX_EOL); // Prints string

print(is_array([1, 2, 3]) . LX_EOL); // Prints true

function no_return() {
    $n = .1;
}

print(is_void(no_return()) . LX_EOL); // Prints true

print(is_defined(LX_EOL) . LX_EOL); // Prints true
```

---

### 3.1 Truthiness

In conditional contexts:

Falsy values:

- `undefined`
- `void`
- `null`
- `false`
- `0`
- `0.0`
- `""` empty string
- `[]` empty arrays

All other values are truthy.

```php
function no_return() {
    $n = 0;
}

$a = [[], "", .0, 0, false, null, no_return(), $z];

for ($i = count($a) - 1; $i >= 0 ; $i--) {
    print(type($a[$i]) . ": ");
    if ($a[$i]) print("true" . LX_EOL);
    else print("false" . LX_EOL);
}

/* Will print
undefined: false
void: false
null: false
bool: false
int: false
float: false
string: false
array: false
*/
```

Lx compared to PHP

| expr             | Lx    | PHP 7.4 | PHP 8.2 |
|------------------|-------|---------|---------|
| "0" == false     | true  | true    | true    |
| "" == false      | true  | true    | true    |
| "abc" == 0       | false | true    | false   |
| "10abc" == 10    | false | true    | false   |
| null == 0        | false | true    | true    |
| "0e12345" == "0" | false | true    | true    |

---

## 4. Literals

### 4.1 Integer literals

```php
123 // base 10, decimal integer
-123 // negative decimal integer
0xFF // base 16, hexadecimal integer
0b1010 // base 2, binary integer
0644 // base 8, octal integer
```

Note: integers with a leading `0` are parsed as octal.

---

### 4.2 Floating-point literals

```lx
.0
1.5
-2.
1e3
```

```php
$a = [.0, 1.5, -2., 1e3];

for ($i = 0; $i < count($a); $i++)
    print(type($a[$i]) . LX_EOL);

/* Will print
float
float
float
float
*/
```

---

### 4.3 String literals

Single-quoted and double-quoted strings are supported.

```
"text\n"
'text'
```

Double-quoted strings support escape sequences:  
`\n`, `\t`, `\r`, `\"`, `\\`, `\$` and `\xnn` (where `nn` is a 1 octet hexadecimal code between 00 and FF).

Single-quoted strings only interpret `\\` and `\'`.

**Double-quoted strings support interpolation.**

- `$name` interpolates a variable.
- `${expr}` interpolates a full expression. If the expression starts with an identifier, the leading `$` is implied.

```php
$a = "Hello";
$b = [10, 20];
$fruits = ["a" => "lemon", "b" => "banana"];
print("$a world\n");                 // Prints Hello world
print("b=${b[1]}\n");                // Prints b=20
print("a=${upper($a)}\n");           // Prints HELLO
print("a fruit=${fruits['a']}\n");   // Prints a fruit=lemon
print("b fruit=${fruits[\"b\"]}\n"); // Prints b fruit=banana
print("cost=\$5\n");                 // Prints cost=$5
print('it\'s ok' . "\n");            // Prints it's ok
```

To combine strings with variable values or expressions, explicit string concatenation using the `.` operator can also be used, or functions like `sprintf()` or `printf()`.

```php
$a = "Hello";
$b = 'World';
print($a . " " . $b . "\n"); // Prints Hello World
print("$a $b\n"); // Prints Hello World
printf("%s %s\n", $a, $b); // Prints Hello World
```

---

### 4.4 Boolean and null literals

```
true
false
null
undefined
void
```

---

### 4.5 Array literals

Arrays may be indexed, associative, or mixed.

```
[]
[1, 2, 3]
["key" => "value", 2 => "x"]
```

Improved canonical form (supported):

```php
$a = [
    "clé1" => "valeur1",
    "clé2" => "valeur2"
];
```

Rules:

- Keys may be integers or strings.
- Elements without explicit keys receive increasing integer keys starting at 0.
- Explicit numeric keys may advance the next automatic index.
- Trailing commas are not accepted.

---

## 5. Expressions and Operators

Expressions yield values and may appear in assignments, conditions, or as standalone statements.

### 5.1 Arithmetic and concatenation

```
$a + $b  // Addition
$a - $b  // Subtraction
$a * $b  // Multiplication
$a / $b  // Division
$a % $b  // Modulo
$a ** $b // Exponentiation
$a . $b  // Concatenation
```

Division or modulo by zero raises a runtime error.

---

### 5.2 Comparison and equality

Weak comparison:

```
$a == $b
$a != $b
```

Strict comparison:

```
$a === $b
$a !== $b
```

Arrays compared with `===` are compared by identity.

```php
$a = [1, 2];
$b = [1, 2];
$c = $a;

print($a === $b, "\n"); // false (different instances)
print($a === $c, "\n"); // true  (same instance)
```

---

### 5.3 Increment and decrement

```
$a++;
$a--;
++$a;
--$a;
```

Works on variables and array elements.  
Postfix returns the previous value; prefix returns the updated value.

### 5.4 Logical operators (!, &&, ||)

Logical operators are described separately because they affect expression evaluation order through short-circuit semantics.

Lx provides logical NOT (`!`), AND (`&&`) and OR (`||`) operators for conditional evaluation.

These operators evaluate their operands according to standard truthiness rules and use **short-circuit evaluation**.

---

##### Logical AND (`&&`)

`expr1 && expr2`

- `expr1` is evaluated first.

- If `expr1` is falsy, `expr2` is **not evaluated**.

- The result is a boolean value.

Example:

```php
$a = 0;
$b = 1;
print($a && $b); // false
```

---

##### Logical OR (`||`)

`expr1 || expr2`

- `expr1` is evaluated first.

- If `expr1` is truthy, `expr2` is **not evaluated**.

- The result is a boolean value.

Example:

```php
$a = 0;
$b = 1;
print($a || $b); // true
```

---

##### Logical NOT (`!`)

`!expr`

- Evaluates `expr` and negates its truthiness.
- The result is a boolean value.

Example:

```php
$a = 0;
print(!$a); // true
```

---

##### Short-circuit behavior

Because of short-circuit evaluation, the right-hand expression may not be executed.

Example:

```php
$a = 0;
$a != 0 && print("never printed");
$a == 0 || print("never printed");
```

---

##### Notes

- Logical operators always return a boolean value.

- Operand evaluation follows the truthiness rules described in **Types and Values**.

- Short-circuit behavior can be used to guard expressions safely.

---

### 5.5 Bitwise and shift operators

```
~  &  ^  |  <<  >>
```

Operands are converted to integers.

Example:

```php
print((5 & 3) . "\n");  // 1
print((5 | 3) . "\n");  // 7
print((5 << 1) . "\n"); // 10
```

---

### 5.6 Ternary operator

```
cond ? expr1 : expr2
```

Example:

```php
print($a > 0 ? "yes" : "no");
```

---

### 5.7 Destructuring assignment

You can assign multiple variables from an array value using bracket syntax:

```php
function f() {
    return [10, 20, 30];
}

[$a, $b, $c] = f();
print($a . "," . $b . "," . $c . "\n"); // 10,20,30
```

Missing values become `undefined`, and only numeric indexes are read (0, 1, 2...).

```php
[$x, $y] = [1];
print($x . "," . $y . "\n"); // 1,undefined
```

Targets may also be array elements:

```php
$arr = ["a" => 1, "b" => 2];
[$arr["a"], $arr["b"]] = [10, 20];
```

---

## 6. Control Structures

### 6.1 Conditional execution

```php
if ($a == 1) {
    print("one\n");
} else {
    print("other\n");
}

if ($b == 0) {
    print("one\n");
} else if ($b == 2) {
    print("two\n");
} else {
    print("other\n");
}
```

Single-statement forms are allowed.

```php
if (x > 0) printf("x is positive\n");
````

---

### 6.2 Loops

#### while

```php
while ($i < 3) {
    print($i);
    $i++;
}
```

#### do / while

```php
do {
    print($i);
    $i++;
} while ($i < 3);
```

#### for

```php
for ($i = 0; $i < 3; $i++) {
    print($i);
}
```

Prefix increments are also supported in the loop clause:

```php
for ($i = 0; $i < 3; ++$i) {
    print($i);
}
```

#### foreach

Arrays:

```php
foreach ([1, 2, 3] as $value) {
    print($value . LX_EOL);
}

foreach (["a" => 1, "b" => 2, "c" => 3] as $key => $value) {
    print($key . ":" . $value . LX_EOL);
}
```

Strings:

```php
foreach ("abc" as $i => $ch) {
    print($i . ":" . $ch . LX_EOL);
}
```

#### switch / case

```php
switch ($x) {
    case 1:
        print("one");
        break;
    case 2:
        print("two");
        break;
    default:
        print("other");
        break;
}
```

Optional strict mode:

```php
switch ($x, true) {
    case 1:
        print("one");
        break;
    default:
        print("other");
        break;
}
```

When `strict` is `true`, comparisons use strict equality (`===`).
When omitted or `false`, comparisons use weak equality (`==`).

---

### 6.3 Loop control

```php
break;
continue;
```

Using `break` or `continue` outside loops raises a runtime error.

---

## 7. Arrays

Arrays are associative containers with integer indexes or string keys.

```php
$a = [];
$a[0] = 10;
$a["x"] = 20;


$a = [1, 2, 3];
$b = ["key" => "value", 2 => "x"];
```

Nested assignment auto-creates intermediate arrays:

```php
$a["nested"]["y"] = 30;

print($a["nested"][keys($a["nested"])[0]] . "\n"); // 30 😄
```

Rules:

- Reading a missing key, or an out of range index, returns `undefined`.
- Indexing a non-array (except string) returns `undefined`.
- String indexing returns a one-character string or `undefined`.
- Appending with `$array[] = value` adds to the next numeric index.
- Index assignment on a non-array raises a runtime error.
- Cyclic array reference assignment raises a runtime error.

---

## 8. Functions

### 8.1 Defining functions

```php
function add($a, $b) {
    return $a + $b;
}
```

Parameters may have default values, which makes them optional at call time.
Defaults are evaluated in the function's local environment (so earlier parameters
are available).

```php
function greet($name = "world") {
    return "hello " . $name;
}

function sum2($a, $b = $a) {
    return $a + $b;
}

print(greet() . "\n");   // hello world
print(sum2(4) . "\n");   // 8
```

Parameters may be written with or without `$`.

```php
function add(a, b) {
    return $a + $b;
}
```

---

### 8.2 Calling functions and return values

```php
print(add(2, 3));
```

If a function ends without `return`, it returns `void`.  
When printed, `void` produces an empty string.

---

### 8.3 Scope and lifetime

Functions execute in a local environment. Variables are **local by default**.
To access or modify globals, you must declare them explicitly with `global`.

```php
function inc() {
    global $g;
    $g = $g + 1;
}

$g = 1;

inc();
print($g . "\n"); // 2
```

Without `global`, assignments create or update **local** variables only:

```php
function inc_local() {
    $g = $g + 1; // local $g (starts as undefined)
}

$g = 1;

inc_local();
print($g . "\n"); // 1
```

---

#### Lifetime rules

- Variables created inside a function exist only for the duration of that call.
- Global variables remain available throughout the script.
- Loop variables remain accessible after the loop finishes.

---

## 9. Built-in and extended functions

Lx provides a set of built-in and extended functions.

See [Lx Functions Reference](lx_functions_reference.md) for details.

---

## 10. Predefined constants

Lx provides a set of predefined constants.

See [Lx Predefined constants](lx_predefined_constants.md) for details.

---

## 11. Magic constants

Lx supports a small set of PHP-like magic constants:

- **`__LINE__`**: Current line number in the source file.
- **`__FILE__`**: Full path to the current file, with symlinks resolved.
- **`__DIR__`**: Directory of the current file (no trailing slash unless root).
- **`__FUNCTION__`**: Current function name, or an empty string outside a function.

Example:

```php
print(__LINE__ . "\n");
print(__FILE__ . "\n");
print(__DIR__ . "\n");
print(__FUNCTION__ . "\n");
```

---

## 12. Predefined variables

Lx provides two predefined variables when running a script:

- **`$argc`**: Number of arguments passed to the script. When running a file, it includes the script path.
- **`$argv`**: Array of arguments passed to the script. When running a file, the script path is at index 0.

Example:

```php
print($argc . "\n");
print($argv[0] . "\n");
```

---

## 13. Memory management

Values are reference-counted. Arrays are additionally tracked by a periodic mark-and-sweep pass.

The `unset` statement provides an explicit way to remove a binding and release the associated memory.

`unset(target);`

The target must be:

- a variable (including array), or

- an indexed array element.

---

### Effect of `unset`

When `unset` is applied:

- the variable or array element binding is removed from the current scope,

- the value becomes unreachable,

- the associated memory can be released by the runtime.

Example:

```php
$a = [1, 2, 3];
unset($a);
print($a . "\n"); // undefined
```

After `unset`, the variable no longer exists and evaluates to `undefined` when accessed.

---

### Unsetting array elements

`unset` can be used to remove a specific element from an array without destroying the array itself.

```php
$arr = ["x" => 42, "y" => 10];
unset($arr["x"]);
print($arr["x"] . "\n"); // undefined
```

Only the specified element is removed; other elements remain unchanged.

---

### Scope and safety rules

- `unset` affects only the current scope.

- It does not create or modify variables in enclosing scopes unless the variable is declared `global`.

- `unset` does not return a value.

- Attempting to unset an invalid target results in an error.

---

### Notes

- `unset` is the primary mechanism for explicit memory release in Lx.

- Variables are never implicitly destroyed.

- Memory management in Lx is explicit and deterministic.

---

## 14. Extensions

Lx exposes a C extension API (see `lx_ext.h`) to register functions, constants, and variables.
Extensions are initialized at startup and run in the global environment.

Example:

```c
#include "lx_ext.h"

static Value n_hello(Env *env, int argc, Value *argv) {
    (void)env; (void)argc; (void)argv;
    return value_string("hello");
}

static void my_module_init(Env *global) {
    lx_register_function("hello", n_hello);
    lx_register_constant(global, "MY_CONST", value_int(123));
    lx_register_variable(global, "greeting", value_string("hi"));
}

void register_my_module(void) {
    lx_register_module(my_module_init);
    lx_register_extension("my_module");
}
```

See [Adding extensions to Lx](lx_adding_extensions.md) for more details.
See [Lx CGI wrapper](lx_cgi.md) for Apache/CGI usage.

Available extensions included with Lx:

- **fs**: [`file_get_contents`](functions/file_get_contents.md), [`file_put_contents`](functions/file_put_contents.md), [`file_exists`](functions/file_exists.md), [`file_size`](functions/file_size.md), [`is_dir`](functions/is_dir.md), [`is_file`](functions/is_file.md), [`mkdir`](functions/mkdir.md), [`rmdir`](functions/rmdir.md), [`unlink`](functions/unlink.md), [`copy`](functions/copy.md), [`cp`](functions/cp.md), [`rename`](functions/rename.md), [`mv`](functions/mv.md), [`chmod`](functions/chmod.md), [`pwd`](functions/pwd.md), [`pathinfo`](functions/pathinfo.md), [`list_dir`](functions/list_dir.md)
- **env**: [`env_get`](functions/env_get.md), [`env_set`](functions/env_set.md), [`env_unset`](functions/env_unset.md), [`env_list`](functions/env_list.md)
- **json**: [`json_encode`](functions/json_encode.md), [`is_json`](functions/is_json.md), [`json_decode`](functions/json_decode.md)
- **serializer**: [`serialize`](functions/serialize.md), [`unserialize`](functions/unserialize.md)
- **hex**: [`bin2hex`](functions/bin2hex.md), [`hex2bin`](functions/hex2bin.md)
- **blake2b**: [`blake2b`](functions/blake2b.md)
- **time**: [`time`](functions/time.md), [`date`](functions/date.md), [`gmdate`](functions/gmdate.md), [`mktime`](functions/mktime.md), [`sleep`](functions/sleep.md), [`usleep`](functions/usleep.md)
- **utf8**: [`glyph_count`](functions/glyph_count.md), [`glyph_at`](functions/glyph_at.md)
- **sqlite**: [`pdo_sqlite_open`](functions/pdo_sqlite_open.md), [`pdo_query`](functions/pdo_query.md), [`pdo_prepare`](functions/pdo_prepare.md), [`pdo_execute`](functions/pdo_execute.md), [`pdo_fetch`](functions/pdo_fetch.md), [`pdo_fetch_all`](functions/pdo_fetch_all.md), [`pdo_last_insert_id`](functions/pdo_last_insert_id.md), [`pdo_close`](functions/pdo_close.md)

See [Lx Functions Reference](lx_functions_reference.md) for details about the functions.

---

## 15. Errors and Limitations

Errors stop execution immediately and are printed as a single line:

```
error <code> line <line>:<col>: <message>
```

Common errors include:

- Parse errors
- Undefined function
- Division or modulo by zero
- Invalid index assignment
- Invalid `unset` target
- `break`/`continue` outside loops
- Cyclic array reference

Limitations:

- No trailing commas in array literals
- Array to string conversion yields `"array"`
- There is no `echo` language construct like in PHP, the function `print()` reminds it yet.
