# Lx Language — User Manual

![](lx_mascot.svg)

---

## 1. Introduction

### 1.1 What is Lx?

Lx is a general-purpose interpreted, dynamically typed, scripting language, with a PHP-like surface syntax.

Programs are parsed and executed directly by the interpreter.

Lx scripts are composed of statements terminated by semicolons. Blocks are delimited with `{ ... }`. Expressions follow a conventional operator precedence model.

This manual describes **only the behavior currently implemented**.

**Name origin**

The name **Lx** originates from the author’s name, **Alex**, pronounced using only the letters **L** and **X**. 

It reflects a deliberate simplification: a short, minimal name aligned with the language’s emphasis on clarity, explicitness, and reduced complexity.

**Mascot choice**

The Lx language mascot is a **lynx**.  
The lynx was chosen for its symbolism: sharp vision, precision, and quiet attentiveness. These traits reflect Lx’s design goals—clarity, explicit behavior, and the absence of hidden mechanisms.

The lynx also echoes the name **Lx** itself (derived from *Alex*, pronounced using the letters *L* and *X*), reinforcing a strong and simple identity. Its minimalist representation aligns with the language’s focus on frugality and readability, even at very small scales such as icons.

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
    print("one");
} else {
    print("other");
}
```

Single-statement forms are allowed.

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

Restrictions:

- The init and step expressions must be assignments to a variable or `++/--`.

#### foreach

Arrays:

```php
foreach ($arr as $value) {
    print($value . LX_EOL);
}

foreach ($arr as $key => $value) {
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

Arrays are associative containers with integer or string keys.

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

print($a["nested"][keys($a["nested"])[0]] . "\n"); // 30
```

Rules:

- Reading a missing key returns `undefined`.
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

## 9. Memory management

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

## 10. Built-in Functions

Lx provides a small set of built-in functions for output, string manipulation, array inspection, and basic type handling.  
All functions described below are globally available.

### Available built-in functions

Output and formatting:

`print`, `printf`, `sprintf`, `lx_info`

Includes:

`include`, `include_once`

Types and inspection:

`type`, `is_null`, `is_bool`, `is_int`, `is_float`, `is_string`, `is_array`, `is_defined`, `is_undefined`, `is_void`

Numeric and math:

`abs`, `min`, `max`, `round`, `floor`, `ceil`, `sqrt`, `pow`, `exp`, `log`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `rand`, `srand`, `clamp`, `pi`, `sign`, `deg2rad`, `rad2deg`

Strings:

`strlen`, `substr`, `trim`, `ltrim`, `rtrim`, `ucfirst`, `strtolower`, `strtoupper`, `strpos`, `strrpos`, `strcmp`, `str_replace`, `str_contains`, `starts_with`, `ends_with`, `ord`, `chr`, `slip`/`explode`, `join`/`implode`

Arrays:

`keys`, `key_exists`, `values`, `in_array`, `push`, `pop`, `shift`, `unshift`, `merge`, `slice`, `splice`, `reverse`

Casting helpers:

`int`, `float`, `str`

---

### include / include_once

```php
include("lib.lx");
include_once("lib.lx");
```

`include` parses and executes another Lx file in the current environment.  
`include_once` executes a file only on the first include attempt.  
If the file cannot be read, a runtime error is raised.

---

### print

```php
print(value);
```

Outputs the string representation of `value`.

- Accepts any value.
- Does not append a newline automatically.
- Returns `void`.

Example:

```php
print("Hello world\n");
```

---

### printf

```php
printf(format, ...args);
```

Outputs a formatted string using `printf`-style formatting.

`printf` behaves like `print(sprintf(format, ...args))`.  
For a detailed description of the formatting syntax and supported specifiers, see **`sprintf`**.

Example:

```php
printf("Integer: %03d\n", -1);
```

Return value:

- `void`

---

### sprintf

```php
sprintf(format, ...args);
```

Builds and returns a formatted string using `printf`-style formatting.

The format string controls how the given arguments are converted and inserted into the resulting string.

Example:

```php
$s = sprintf("The result is: %.2f", 1.234);
print($s . "\n");
```

Return value:

- `string`

---

#### Format specification syntax

Each format specifier in `sprintf` and `printf` follows this general form:

```
%[flags][width][.precision]specifier
```

Not all components are required, and they may be combined depending on the specifier.

---

#### Specifier components

##### Flags

Flags modify the formatting behavior:

- `0`  
  Pads the result with leading zeros instead of spaces.

Example:

```php
sprintf("%05d", 42);   // "00042"
```

---

##### Width

The width specifies the **minimum number of characters** to output.

If the formatted value is shorter, it is padded (by default with spaces, or with zeros if the `0` flag is used).

Example:

```php
sprintf("%5d", 42);   // "   42"
sprintf("%05d", 42);  // "00042"
```

---

##### Precision

Precision is introduced by a dot (`.`) and has different meanings depending on the specifier:

- For floating-point numbers, it specifies the number of digits after the decimal point.
- For integers, it specifies the minimum number of digits.
- For strings, it specifies the maximum number of characters.

Examples:

```php
sprintf("%.2f", 3.14159);  // "3.14"
sprintf("%.3d", 7);        // "007"
sprintf("%.4s", "abcdef"); // "abcd"
```

---

#### Supported format specifiers

| Specifier | Description                                                                                                                            |
| ---------:| -------------------------------------------------------------------------------------------------------------------------------------- |
| `%s`      | String                                                                                                                                 |
| `%c`      | Character. The argument may be the first character of a string or an integer interpreted as an ASCII code. An empty string is ignored. |
| `%d`      | Signed decimal integer                                                                                                                 |
| `%i`      | Signed integer                                                                                                                         |
| `%u`      | Unsigned integer                                                                                                                       |
| `%x`      | Unsigned hexadecimal integer (lowercase)                                                                                               |
| `%X`      | Unsigned hexadecimal integer (uppercase)                                                                                               |
| `%o`      | Unsigned octal integer                                                                                                                 |
| `%f`      | Floating-point number                                                                                                                  |
| `%F`      | Floating-point number                                                                                                                  |
| `%e`      | Scientific notation (lowercase)                                                                                                        |
| `%E`      | Scientific notation (uppercase)                                                                                                        |
| `%g`      | Shortest representation (`%f` or `%e`)                                                                                                 |
| `%G`      | Shortest representation (`%F` or `%E`)                                                                                                 |
| `%%`      | Literal percent sign. No argument required.                                                                                            |

---

#### Combined examples

```php
sprintf("%8.2f", 3.1);    // "    3.10"
sprintf("%08.2f", 3.1);   // "00003.10"
sprintf("%6s", "lx");     // "    lx"
sprintf("%.3s", "lynx");  // "lyn"
sprintf("%04x", 15);      // "000f"
```

---

#### Notes and limitations

- Width and precision must be numeric literals.
- Dynamic width or precision using `*` is not supported.
- Unsupported or invalid format sequences are left unchanged in the output.

---

### strlen

```php
strlen(string);
```

Returns the length of a string in characters.

Example:

```php
print(strlen("abcd"));    // 4
print(strlen([0, 1, 2])); // 0
```

Return value:

- `int`

---

### count

```php
count(array);
```

Returns the number of elements in an array.

If `value` is not an array, `count` returns `0`.

Example:

```php
print(count([1, 2, 3])); // 3
print(count("abc"));     // 0
```

Return value:

- `int`

---

### substr

```php
substr(string, start [, length]);
```

Returns a portion of a string.

- `start` specifies the starting index (0-based).
- If `length` is provided, at most `length` characters are returned.

Example:

```php
print(substr("abcd", 1, 2)); // "bc"
```

Return value:

- `string`

---

### type

```php
type(value);
```

Returns the runtime type of `value` as a string.

Example:

```php
print(type([]));  // "array"
print(type(123)); // "int"
```

Return value:

- `string`

---

### ord

```php
ord(string);
```

Returns the numeric code of the first character of `string`.

Example:

```php
print(ord("A")); // 65
```

Return value:

- `int`

---

### chr

```php
chr(code);
```

Returns a one-character string corresponding to the given numeric code.

Example:

```php
print(chr(0x41)); // "A"
```

Return value:

- `string`

---

### keys

```php
keys(array);
```

Returns an array containing all the keys of the given array.

The returned array contains the keys in the order they appear in the array.

Example:

```php
$a = ["x" => 10, "y" => 20, 3 => "z"];

$k = keys($a);

for ($i = 0; $i < count($k); $i++)
    print($k[$i] . ": " . $a[$k[$i]] . "\n");

/* Will print
x: 10
y: 20
3: z
*/
```

Return value:

- `array`

Notes:

- If the argument is not an array, an empty array is returned.

- Both integer and string keys are included.

---

## 11. Predefined Constants

Lx provides a set of predefined constants that expose information about the runtime environment and numeric limits of the current Lx binary.  
These constants are available globally and cannot be modified.

---

### Version and Environment

- **`LX_VERSION`** (`string`)  
  The version string of the running Lx interpreter.  
  Example: `"1.0"`

- **`LX_EOL`** (`string`)  
  The end-of-line sequence used by Lx.  
  Currently defined as a newline character (`"\n"`).

---

### Integer Limits

- **`LX_INT_MAX`** (`int`)  
  The largest integer value supported by this Lx binary.

- **`LX_INT_MIN`** (`int`)  
  The smallest integer value supported by this version of Lx.

- **`LX_INT_SIZE`** (`int`)  
  The size of an integer, in bytes, in this version of Lx.

These values reflect the integer representation used internally by the interpreter.

---

### Floating-Point Limits

- **`LX_FLOAT_DIG`** (`int`)  
  The number of decimal digits that can be rounded and returned for a floating-point number without loss of precision.

- **`LX_FLOAT_EPSILON`** (`float`)  
  The smallest positive floating-point value such that `1.0 + x != 1.0`.

- **`LX_FLOAT_MIN`** (`float`)  
  The smallest positive floating-point value supported.  
  To obtain the smallest negative floating-point value, use `-LX_FLOAT_MAX`.

- **`LX_FLOAT_MAX`** (`float`)  
  The largest floating-point value supported.

These constants describe the floating-point characteristics of the current Lx implementation.

---

### Math constants

- **`M_PI`** (`float`)
- **`M_E`** (`float`)
- **`M_LN2`** (`float`)
- **`M_LN10`** (`float`)
- **`M_LOG2E`** (`float`)
- **`M_LOG10E`** (`float`)
- **`M_SQRT2`** (`float`)
- **`M_SQRT1_2`** (`float`)

These constants use standard double-precision values.

---

## 13. Extensions

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

Built-in extensions:

- **fs**: `file_get_contents`, `file_put_contents`, `file_exists`, `file_size`, `is_dir`, `is_file`, `mkdir`, `rmdir`, `unlink`, `pathinfo`, `list_dir`
- **json**: `json_encode`, `json_decode`
- **serializer**: `serialize`, `unserialize`
- **hex**: `bin2hex`, `hex2bin`

---

## 14. Errors and Limitations

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

## 15. Undefined or Unclear Behavior

- Floating-point formatting depends on `%g`

This behavior is undefined or unclear in the current implementation.
