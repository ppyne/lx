/**
 * @file ext_blake2b.c
 * @brief BLAKE2b extension module.
 */
#include "lx_ext.h"
#include "blake2.h"
#include <stdlib.h>
#include <string.h>

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static Value to_hex(const unsigned char *buf, size_t len) {
    static const char hex[] = "0123456789abcdef";
    size_t out_len = len * 2;
    char *out = (char *)malloc(out_len + 1);
    if (!out) return value_string("");
    for (size_t i = 0; i < len; i++) {
        out[i * 2] = hex[(buf[i] >> 4) & 0x0F];
        out[i * 2 + 1] = hex[buf[i] & 0x0F];
    }
    out[out_len] = '\0';
    Value v = value_string(out);
    free(out);
    return v;
}

static Value to_base64(const unsigned char *buf, size_t len) {
    size_t out_len = ((len + 2) / 3) * 4;
    char *out = (char *)malloc(out_len + 1);
    if (!out) return value_string("");

    size_t i = 0;
    size_t j = 0;
    while (i < len) {
        size_t rem = len - i;
        unsigned char a = buf[i++];
        unsigned char b = (rem > 1) ? buf[i++] : 0;
        unsigned char c = (rem > 2) ? buf[i++] : 0;

        out[j++] = base64_table[(a >> 2) & 0x3F];
        out[j++] = base64_table[((a & 0x03) << 4) | ((b >> 4) & 0x0F)];
        out[j++] = (rem > 1) ? base64_table[((b & 0x0F) << 2) | ((c >> 6) & 0x03)] : '=';
        out[j++] = (rem > 2) ? base64_table[c & 0x3F] : '=';
    }
    out[out_len] = '\0';
    Value v = value_string(out);
    free(out);
    return v;
}

static Value n_blake2b(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 1 || argv[0].type != VAL_STRING) return value_undefined();
    const unsigned char *in = (const unsigned char *)(argv[0].s ? argv[0].s : "");
    size_t in_len = strlen((const char *)in);

    int out_len = 4;
    if (argc >= 2) out_len = (int)value_to_int(argv[1]).i;
    if (out_len < 1) out_len = 1;
    if (out_len > 64) out_len = 64;

    int use_base64 = 0;
    if (argc >= 3) use_base64 = value_is_true(argv[2]);

    unsigned char out[64];
    if (blake2b(out, (size_t)out_len, in, in_len, NULL, 0) != 0) {
        return value_undefined();
    }

    if (use_base64) return to_base64(out, (size_t)out_len);
    return to_hex(out, (size_t)out_len);
}

static void blake2b_module_init(Env *global){
    lx_register_function("blake2b", n_blake2b);
    (void)global;
}

void register_blake2b_module(void) {
    lx_register_extension("blake2b");
    lx_register_module(blake2b_module_init);
}
