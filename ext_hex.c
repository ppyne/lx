/**
 * @file ext_hex.c
 * @brief Hex encoding extension module.
 */
#include "lx_ext.h"
#include <stdlib.h>
#include <string.h>

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static Value n_bin2hex(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_string("");
    const unsigned char *s = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t len = strlen((const char *)s);
    char *out = (char *)malloc(len * 2 + 1);
    if (!out) return value_string("");
    static const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[s[i] >> 4];
        out[i * 2 + 1] = hex[s[i] & 0x0F];
    }
    out[len * 2] = '\0';
    Value v = value_string(out);
    free(out);
    return v;
}

static Value n_blob_to_hex(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_BLOB || !argv[0].blob) return value_string("");
    const unsigned char *s = argv[0].blob->data;
    size_t len = argv[0].blob->len;
    char *out = (char *)malloc(len * 2 + 1);
    if (!out) return value_string("");
    static const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[s[i] >> 4];
        out[i * 2 + 1] = hex[s[i] & 0x0F];
    }
    out[len * 2] = '\0';
    Value v = value_string(out);
    free(out);
    return v;
}

static Value n_hex2bin(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    if (len % 2 != 0) return value_undefined();
    char *out = (char *)malloc(len / 2 + 1);
    if (!out) return value_undefined();
    for (size_t i = 0; i < len; i += 2) {
        int hi = hex_val(s[i]);
        int lo = hex_val(s[i + 1]);
        if (hi < 0 || lo < 0) { free(out); return value_undefined(); }
        out[i / 2] = (char)((hi << 4) | lo);
    }
    out[len / 2] = '\0';
    Value v = value_string_n(out, len / 2);
    free(out);
    return v;
}

static Value n_blob_from_hex(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1 || argv[0].type != VAL_STRING) return value_undefined();
    const char *s = argv[0].s ? argv[0].s : "";
    size_t len = strlen(s);
    if (len % 2 != 0) return value_undefined();
    Blob *out = blob_new(len / 2);
    if (!out) return value_undefined();
    for (size_t i = 0; i < len; i += 2) {
        int hi = hex_val(s[i]);
        int lo = hex_val(s[i + 1]);
        if (hi < 0 || lo < 0) { blob_free(out); return value_undefined(); }
        out->data[i / 2] = (unsigned char)((hi << 4) | lo);
    }
    Value v; v.type = VAL_BLOB; v.blob = out;
    return v;
}

static void hex_module_init(Env *global){
    lx_register_function("bin2hex", n_bin2hex);
    lx_register_function("blob_to_hex", n_blob_to_hex);
    lx_register_function("hex2bin", n_hex2bin);
    lx_register_function("blob_from_hex", n_blob_from_hex);
    (void)global;
}

void register_hex_module(void) {
    lx_register_extension("hex");
    lx_register_module(hex_module_init);
}
