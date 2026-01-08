# Lx Language — User Manual

<p align="center"><img src="lx_mascot.svg" width="128" alt="Lx mascot"></p>

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
```

Comments may appear anywhere whitespace is allowed.

---

### 2.3 Variables

Variables are prefixed with `$` and are created by assignment.

```php
$a = 10;
```

Assignments update an existing variable in the nearest enclosing scope.  
If no binding exists, a new variable is created in the current scope.

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

$str = "Hello world";
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

Only double-quoted strings are supported.

```
"text\n"
```

Supported escape sequences:  
`\n`, `\t`, `\r`, `\"`, `\\` and `\xnn` (where `nn` is a 1 octet hexadecimal code between 00 and FF).

Single-quoted strings are not supported.

**String literals are not interpolated.**

Except for supported escape sequences, the content of a string literal is treated as plain text.  Variables and expressions are **not** evaluated inside strings.

To combine strings with variable values or expressions, explicit string concatenation using the `.` operator must be used, or functions like `sprintf()` or `printf()`.

```php
$a = "Hello";
$b = "World";
print($a . " " . $b . "\n"); // Prints Hello World

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
$a + $b
$a - $b
$a * $b
$a / $b
$a % $b
$a ** $b
$a . $b
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

Lx provides logical AND (`&&`) and logical OR (`||`) operators for conditional evaluation.

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

Functions execute in a local environment that is linked to the calling environment.

When a variable is assigned inside a function, Lx resolves the assignment as follows:

- If a variable binding with the same name exists in an enclosing scope, that binding is updated.

- Otherwise, a new variable is created in the function’s local scope.

This resolution rule applies uniformly to all assignments.

---

#### Updating an existing binding

If a variable already exists in the calling scope, assignments inside the function update that variable.

Example:

```php
function inc() {
    $g = $g + 1;
}

$g = 1;
inc();
print($g . "\n"); // 2
```

In this example, `$g` exists in the enclosing scope.  
The assignment inside `inc` updates the existing binding.

---

#### Assignment when no binding exists

If no variable binding exists in any enclosing scope, the assignment creates a **local** variable inside the function.

The variable is evaluated before assignment. If it does not exist, its value is `undefined`.

Example:

```php
function f() {
    $v += 1;
}

f();
print($v . "\n"); // undefined
```

In this example:

- `$v` has no existing binding in the enclosing scope.

- `$v` inside the function is evaluated as `undefined`.

- The assignment creates a local `$v` inside the function.

- No variable `$v` exists in the outer scope after the function call.

---

#### Lifetime rules

- Variables created inside a function exist only for the duration of that function call.

- Variables in enclosing scopes are not implicitly created or modified.

- Loop variables remain accessible after the loop finishes.

---

#### Notes

- Lx does not implicitly create global variables.

- Variable resolution is explicit and deterministic.

- No scope is modified unless a binding already exists.

---

## 9. Built-in and extended functions

Lx provides a set of built-in and extended functions.

See [Lx Functions Reference](lx_functions_reference.md) for details.

---

## 10. Predefined constants

Lx provides a set of predefined constants.

See [Lx Predefined constants](lx_predefined_constants.md) for details.

---

## 11. Memory management

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

- It does not create or modify variables in enclosing scopes.

- `unset` does not return a value.

- Attempting to unset an invalid target results in an error.

---

### Notes

- `unset` is the primary mechanism for explicit memory release in Lx.

- Variables are never implicitly destroyed.

- Memory management in Lx is explicit and deterministic.

---

## 12. Extensions

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

Built-in extensions:

- **fs**: `file_get_contents`, `file_put_contents`, `file_exists`, `file_size`, `is_dir`, `is_file`, `mkdir`, `rmdir`, `unlink`, `pathinfo`, `list_dir`
- **json**: `json_encode`, `json_decode`
- **serializer**: `serialize`, `unserialize`
- **hex**: `bin2hex`, `hex2bin`

---

## 13. Errors and Limitations

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

- No single-quoted strings
- No trailing commas in array literals
- Array to string conversion yields `"array"`

---

## 14. Undefined or Unclear Behavior

- Floating-point formatting depends on `%g`

This behavior is undefined or unclear in the current implementation.
