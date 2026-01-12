/**
 * @file ext_aead.c
 * @brief AEAD (ChaCha20-Poly1305) extension module.
 */
#include "lx_ext.h"
#include "monocypher.h"
#include <string.h>

static int get_bytes(Value v, const uint8_t **data, size_t *len) {
    if (!data || !len) return 0;
    if (v.type == VAL_BLOB && v.blob) {
        *data = v.blob->data;
        *len = v.blob->len;
        return 1;
    }
    if (v.type == VAL_STRING) {
        const char *s = v.s ? v.s : "";
        *data = (const uint8_t *)s;
        *len = strlen(s);
        return 1;
    }
    return 0;
}

static Value n_aead_encrypt(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 3 || argc > 4) return value_undefined();
    const uint8_t *key = NULL;
    const uint8_t *nonce = NULL;
    const uint8_t *plain = NULL;
    const uint8_t *ad = NULL;
    size_t key_len = 0;
    size_t nonce_len = 0;
    size_t plain_len = 0;
    size_t ad_len = 0;
    if (!get_bytes(argv[0], &key, &key_len) || key_len != 32) return value_undefined();
    if (!get_bytes(argv[1], &nonce, &nonce_len) || nonce_len != 24) return value_undefined();
    if (!get_bytes(argv[2], &plain, &plain_len)) return value_undefined();
    if (argc == 4) {
        if (!get_bytes(argv[3], &ad, &ad_len)) return value_undefined();
    }

    Blob *b = blob_new(plain_len + 16);
    if (!b) return value_undefined();
    uint8_t mac[16];
    crypto_aead_lock(b->data, mac, key, nonce, ad, ad_len, plain, plain_len);
    memcpy(b->data + plain_len, mac, 16);
    b->len = plain_len + 16;
    Value out; out.type = VAL_BLOB; out.blob = b;
    return out;
}

static Value n_aead_decrypt(Env *env, int argc, Value *argv){
    (void)env;
    if (argc < 3 || argc > 4) return value_undefined();
    const uint8_t *key = NULL;
    const uint8_t *nonce = NULL;
    const uint8_t *cipher = NULL;
    const uint8_t *ad = NULL;
    size_t key_len = 0;
    size_t nonce_len = 0;
    size_t cipher_len = 0;
    size_t ad_len = 0;
    if (!get_bytes(argv[0], &key, &key_len) || key_len != 32) return value_undefined();
    if (!get_bytes(argv[1], &nonce, &nonce_len) || nonce_len != 24) return value_undefined();
    if (!get_bytes(argv[2], &cipher, &cipher_len) || cipher_len < 16) return value_undefined();
    if (argc == 4) {
        if (!get_bytes(argv[3], &ad, &ad_len)) return value_undefined();
    }

    size_t plain_len = cipher_len - 16;
    const uint8_t *mac = cipher + plain_len;
    Blob *b = blob_new(plain_len);
    if (!b) return value_undefined();
    int mismatch = crypto_aead_unlock(b->data, mac, key, nonce, ad, ad_len, cipher, plain_len);
    if (mismatch != 0) {
        blob_free(b);
        return value_undefined();
    }
    b->len = plain_len;
    Value out; out.type = VAL_BLOB; out.blob = b;
    return out;
}

static void aead_module_init(Env *global){
    lx_register_function("aead_encrypt", n_aead_encrypt);
    lx_register_function("aead_decrypt", n_aead_decrypt);
    (void)global;
}

void register_aead_module(void) {
    lx_register_extension("aead");
    lx_register_module(aead_module_init);
}
