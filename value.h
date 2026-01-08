/**
 * @file value.h
 * @brief Runtime value representation and conversions.
 */
#ifndef VALUE_H
#define VALUE_H

#include <stddef.h>

typedef struct Array Array;

/** Value type tags used by the runtime. */
typedef enum {
    VAL_UNDEFINED = 0, /**< Internal sentinel for missing values. */
    VAL_VOID,          /**< Function returns no value. */
    VAL_NULL,          /**< Null literal. */
    VAL_INT,           /**< Integer value. */
    VAL_FLOAT,         /**< Floating-point value. */
    VAL_BOOL,          /**< Boolean value. */
    VAL_STRING,        /**< Owned string value. */
    VAL_ARRAY          /**< Reference-counted array value. */
} ValueType;

/** Tagged union holding a runtime value. */
typedef struct {
    ValueType type;
    union {
        int     i; /**< Integer payload. */
        double  f; /**< Floating-point payload. */
        int     b; /**< Boolean payload (0/1). */
        char   *s; /**< Owned string buffer. */
        Array  *a; /**< Reference-counted array pointer. */
    };
} Value;

/** @return A VAL_UNDEFINED value. */
Value value_undefined(void);
/** @return A VAL_VOID value. */
Value value_void(void);
/** @return A VAL_NULL value. */
Value value_null(void);
/** @return A VAL_INT value for @p x. */
Value value_int(int x);
/** @return A VAL_FLOAT value for @p x. */
Value value_float(double x);
/** @return A VAL_BOOL value for @p b. */
Value value_bool(int b);
/** @return A VAL_STRING value (copying @p s). */
Value value_string(const char *s);
/** @return A VAL_STRING value copying exactly @p n bytes. */
Value value_string_n(const char *s, size_t n);
/** @return A new empty VAL_ARRAY value. */
Value value_array(void);

/** @return Non-zero if @p v is truthy. */
int   value_is_true(Value v);
/** @return Non-zero if @p v is numeric or boolean. */
int   value_is_number(Value v);

/** @return A copy of @p v (strings copied, arrays retained). */
Value value_copy(Value v);
/** Release resources owned by @p v. */
void  value_free(Value v);

/** @return A VAL_STRING representation (caller owns the string). */
Value value_to_string(Value v);
/** @return Best-effort integer conversion. */
Value value_to_int(Value v);
/** @return Best-effort float conversion. */
Value value_to_float(Value v);
/** @return Best-effort double conversion. */
double value_as_double(Value v);


#endif
