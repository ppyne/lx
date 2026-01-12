/**
 * @file value.h
 * @brief Runtime value representation and conversions.
 */
#ifndef VALUE_H
#define VALUE_H

#include <stddef.h>
#include <stdint.h>
#include "lx_int.h"

typedef struct Array Array;
typedef struct Blob Blob;

/** Value type tags used by the runtime. */
typedef enum {
    VAL_UNDEFINED = 0, /**< Internal sentinel for missing values. */
    VAL_VOID,          /**< Function returns no value. */
    VAL_NULL,          /**< Null literal. */
    VAL_INT,           /**< Integer value. */
    VAL_FLOAT,         /**< Floating-point value. */
    VAL_BOOL,          /**< Boolean value. */
    VAL_BYTE,          /**< Unsigned byte value (0..255). */
    VAL_STRING,        /**< Owned string value. */
    VAL_BLOB,          /**< Binary blob value. */
    VAL_ARRAY          /**< Reference-counted array value. */
} ValueType;

/** Tagged union holding a runtime value. */
typedef struct {
    ValueType type;
    union {
        lx_int_t i; /**< Integer payload. */
        double  f; /**< Floating-point payload. */
        int     b; /**< Boolean payload (0/1). */
        uint8_t byte; /**< Byte payload (0..255). */
        char   *s; /**< Owned string buffer. */
        Blob   *blob; /**< Reference-counted blob pointer. */
        Array  *a; /**< Reference-counted array pointer. */
    };
} Value;

/** Binary blob storage (byte buffer with explicit length). */
struct Blob {
    unsigned char *data;
    size_t len;
    size_t cap;
    int refcount;
};

/** @return A VAL_UNDEFINED value. */
Value value_undefined(void);
/** @return A VAL_VOID value. */
Value value_void(void);
/** @return A VAL_NULL value. */
Value value_null(void);
/** @return A VAL_INT value for @p x. */
Value value_int(lx_int_t x);
/** @return A VAL_FLOAT value for @p x. */
Value value_float(double x);
/** @return A VAL_BOOL value for @p b. */
Value value_bool(int b);
/** @return A VAL_BYTE value for @p b (0..255). */
Value value_byte(unsigned char b);
/** @return A VAL_STRING value (copying @p s). */
Value value_string(const char *s);
/** @return A VAL_STRING value copying exactly @p n bytes. */
Value value_string_n(const char *s, size_t n);
/** @return A VAL_BLOB value copying exactly @p n bytes. */
Value value_blob_n(const unsigned char *data, size_t n);
/** @return A new empty VAL_ARRAY value. */
Value value_array(void);

/** @return A new blob with length @p n (zeroed). */
Blob *blob_new(size_t n);
/** @return A new blob copying @p n bytes from @p data. */
Blob *blob_from_bytes(const unsigned char *data, size_t n);
/** Retain a blob reference. */
void  blob_retain(Blob *b);
/** Release a blob reference. */
void  blob_free(Blob *b);
/** Ensure blob capacity is at least @p cap bytes. */
int   blob_reserve(Blob *b, size_t cap);

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
