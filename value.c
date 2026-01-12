/**
 * @file value.c
 * @brief Value helpers and conversions.
 */
#include "value.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

Value value_undefined(void){ Value v; v.type=VAL_UNDEFINED; return v; }
Value value_void(void){ Value v; v.type=VAL_VOID; return v; }
Value value_null(void){ Value v; v.type=VAL_NULL; return v; }
Value value_int(lx_int_t x){ Value v; v.type=VAL_INT; v.i=x; return v; }
Value value_float(double x){ Value v; v.type=VAL_FLOAT; v.f=x; return v; }
Value value_bool(int b){ Value v; v.type=VAL_BOOL; v.b=!!b; return v; }
Value value_byte(unsigned char b){ Value v; v.type=VAL_BYTE; v.byte=b; return v; }

Value value_string(const char *s){
    Value v; v.type=VAL_STRING;
    v.s = strdup(s ? s : "");
    return v;
}
Value value_string_n(const char *s, size_t n){
    Value v; v.type=VAL_STRING;
    v.s = (char*)malloc(n+1);
    if (!v.s) { v.type=VAL_NULL; return v; }
    memcpy(v.s, s, n);
    v.s[n]=0;
    return v;
}
Value value_blob_n(const unsigned char *data, size_t n){
    Blob *b = blob_from_bytes(data, n);
    Value v; v.type=VAL_BLOB; v.blob = b;
    if (!b) v.type = VAL_NULL;
    return v;
}
Value value_array(void){
    Value v; v.type=VAL_ARRAY;
    v.a = array_new();
    return v;
}

Blob *blob_new(size_t n){
    Blob *b = (Blob *)malloc(sizeof(Blob));
    if (!b) return NULL;
    b->len = n;
    b->cap = n;
    b->refcount = 1;
    if (n == 0) {
        b->data = NULL;
        return b;
    }
    b->data = (unsigned char *)malloc(n);
    if (!b->data) { free(b); return NULL; }
    memset(b->data, 0, n);
    return b;
}

Blob *blob_from_bytes(const unsigned char *data, size_t n){
    Blob *b = blob_new(n);
    if (!b) return NULL;
    if (n > 0 && data) memcpy(b->data, data, n);
    return b;
}

void blob_retain(Blob *b){
    if (b) b->refcount++;
}

void blob_free(Blob *b){
    if (!b) return;
    if (--b->refcount > 0) return;
    free(b->data);
    free(b);
}

int blob_reserve(Blob *b, size_t cap){
    if (!b) return 0;
    if (b->cap >= cap) return 1;
    size_t ncap = b->cap ? b->cap : 8;
    while (ncap < cap) ncap *= 2;
    unsigned char *ndata = (unsigned char *)realloc(b->data, ncap);
    if (!ndata) return 0;
    b->data = ndata;
    b->cap = ncap;
    return 1;
}

int value_is_number(Value v){
    return v.type==VAL_INT || v.type==VAL_FLOAT || v.type==VAL_BOOL || v.type==VAL_BYTE;
}

int value_is_true(Value v){
    switch (v.type){
        case VAL_UNDEFINED: return 0;
        case VAL_VOID:      return 0;
        case VAL_NULL:      return 0;
        case VAL_BOOL:      return v.b != 0;
        case VAL_INT:       return v.i != 0;
        case VAL_FLOAT:     return v.f != 0.0;
        case VAL_BYTE:      return v.byte != 0;
        case VAL_STRING:    return v.s && v.s[0] != 0;
        case VAL_BLOB:      return v.blob && v.blob->len != 0;
        case VAL_ARRAY:     return v.a && v.a->size != 0;
        default:            return 0;
    }
}

Value value_copy(Value v){
    switch (v.type){
        case VAL_STRING: return value_string(v.s);
        case VAL_BLOB:  {
            Value out; out.type = VAL_BLOB; out.blob = v.blob;
            blob_retain(out.blob);
            return out;
        }
        case VAL_ARRAY:  {
            Value out; out.type=VAL_ARRAY;
            out.a = v.a;
            array_retain(out.a);
            return out;
        }
        default: return v; /* POD copy */
    }
}

void value_free(Value v){
    switch (v.type){
        case VAL_STRING: free(v.s); break;
        case VAL_BLOB:  blob_free(v.blob); break;
        case VAL_ARRAY:  array_free(v.a); break;
        default: break;
    }
}

static Value float_to_string(double f) {
    char tmp[128];
    if (isnan(f)) return value_string("nan");
    if (isinf(f)) return value_string(signbit(f) ? "-inf" : "inf");
    if (f == 0.0) return value_string(signbit(f) ? "-0.0" : "0.0");
    double ipart = 0.0;
    if (modf(f, &ipart) == 0.0) {
        snprintf(tmp, sizeof(tmp), "%.0f", f);
        size_t len = strlen(tmp);
        if (len + 2 < sizeof(tmp)) {
            tmp[len] = '.';
            tmp[len + 1] = '0';
            tmp[len + 2] = '\0';
            return value_string(tmp);
        }
    }
    snprintf(tmp, sizeof(tmp), "%.15g", f);
    if (!strchr(tmp, 'e') && !strchr(tmp, 'E')) {
        if (tmp[0] == '.' || tmp[0] == '-') {
            if (tmp[0] == '.' ) {
                memmove(tmp + 1, tmp, strlen(tmp) + 1);
                tmp[0] = '0';
            } else if (tmp[0] == '-' && tmp[1] == '.') {
                memmove(tmp + 2, tmp + 1, strlen(tmp));
                tmp[1] = '0';
            }
        }
    }
    return value_string(tmp);
}

Value value_to_string(Value v){
    char tmp[128];
    switch (v.type){
        case VAL_UNDEFINED: return value_string("undefined");
        case VAL_VOID:      return value_string("");
        case VAL_NULL:      return value_string("null");
        case VAL_BOOL:      return value_string(v.b ? "true" : "false");
        case VAL_INT:       snprintf(tmp,sizeof(tmp),"%" LX_INT_FMT, v.i); return value_string(tmp);
        case VAL_FLOAT:     return float_to_string(v.f);
        case VAL_BYTE:      snprintf(tmp,sizeof(tmp),"%u",(unsigned)v.byte); return value_string(tmp);
        case VAL_STRING:    return value_string(v.s);
        case VAL_BLOB: {
            if (!v.blob || !v.blob->data) return value_string("");
            size_t n = v.blob->len;
            if (n == 0) return value_string("");
            unsigned char *p = memchr(v.blob->data, 0, n);
            if (p) n = (size_t)(p - v.blob->data);
            return value_string_n((const char *)v.blob->data, n);
        }
        case VAL_ARRAY:     return value_string("array");
        default:            return value_string("null");
    }
}

Value value_to_int(Value v){
    switch (v.type){
        case VAL_INT: return v;
        case VAL_BOOL: return value_int(v.b ? 1 : 0);
        case VAL_FLOAT: return value_int((lx_int_t)v.f);
        case VAL_BYTE: return value_int((int)v.byte);
        case VAL_STRING: {
            if (!v.s) return value_int(0);
            char *end = NULL;
            long long n = strtoll(v.s, &end, 10);
            if (end && end != v.s) return value_int((lx_int_t)n);
            return value_int(0);
        }
        case VAL_NULL:
        case VAL_VOID:
        case VAL_UNDEFINED: return value_int(0);
        case VAL_BLOB: return value_int(0);
        default: return value_int(0);
    }
}
Value value_to_float(Value v){
    switch (v.type){
        case VAL_FLOAT: return v;
        case VAL_INT: return value_float((double)v.i);
        case VAL_BOOL: return value_float((double)(v.b ? 1 : 0));
        case VAL_BYTE: return value_float((double)v.byte);
        case VAL_STRING: return value_float(v.s ? atof(v.s) : 0.0);
        case VAL_NULL:
        case VAL_VOID:
        case VAL_UNDEFINED: return value_float(0.0);
        case VAL_BLOB: return value_float(0.0);
        default: return value_float(0.0);
    }
}

double value_as_double(Value v)
{
    switch (v.type) {
        case VAL_INT:   return (double)v.i;
        case VAL_FLOAT: return v.f;
        case VAL_BOOL:  return (double)(v.b ? 1 : 0);
        case VAL_BYTE:  return (double)v.byte;
        case VAL_STRING:
            return v.s ? strtod(v.s, NULL) : 0.0;
        case VAL_NULL:
        case VAL_VOID:
        case VAL_UNDEFINED:
        case VAL_BLOB:
        default:
            return 0.0;
    }
}
