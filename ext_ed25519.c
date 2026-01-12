/**
 * @file ext_ed25519.c
 * @brief Ed25519 signature extension module.
 */
#include "lx_ext.h"
#include "array.h"
#include "monocypher.h"
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

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

static int read_random(uint8_t *out, size_t len) {
    if (!out || len == 0) return 0;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return 0;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, out + off, len - off);
        if (n <= 0) { close(fd); return 0; }
        off += (size_t)n;
    }
    close(fd);
    return 1;
}

static Value make_keypair(const uint8_t seed[32]) {
    uint8_t secret[64];
    uint8_t public[32];
    uint8_t seed_copy[32];
    memcpy(seed_copy, seed, 32);
    crypto_eddsa_key_pair(secret, public, seed_copy);
    Value out = value_array();
    array_set(out.a, key_string("public"), value_blob_n(public, sizeof(public)));
    array_set(out.a, key_string("secret"), value_blob_n(secret, sizeof(secret)));
    return out;
}

static Value n_ed25519_seed_keypair(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_undefined();
    const uint8_t *seed = NULL;
    size_t seed_len = 0;
    if (!get_bytes(argv[0], &seed, &seed_len) || seed_len != 32) return value_undefined();
    return make_keypair(seed);
}

static Value n_ed25519_keypair(Env *env, int argc, Value *argv){
    (void)env;
    (void)argv;
    if (argc != 0) return value_undefined();
    uint8_t seed[32];
    if (!read_random(seed, sizeof(seed))) return value_undefined();
    return make_keypair(seed);
}

static Value n_ed25519_public_key(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 1) return value_undefined();
    const uint8_t *secret = NULL;
    size_t secret_len = 0;
    if (!get_bytes(argv[0], &secret, &secret_len) || secret_len != 64) return value_undefined();
    return value_blob_n(secret + 32, 32);
}

static Value n_ed25519_sign(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 2) return value_undefined();
    const uint8_t *secret = NULL;
    const uint8_t *msg = NULL;
    size_t secret_len = 0;
    size_t msg_len = 0;
    if (!get_bytes(argv[0], &secret, &secret_len) || secret_len != 64) return value_undefined();
    if (!get_bytes(argv[1], &msg, &msg_len)) return value_undefined();
    Blob *sig = blob_new(64);
    if (!sig) return value_undefined();
    crypto_eddsa_sign(sig->data, secret, msg, msg_len);
    sig->len = 64;
    Value out; out.type = VAL_BLOB; out.blob = sig;
    return out;
}

static Value n_ed25519_verify(Env *env, int argc, Value *argv){
    (void)env;
    if (argc != 3) return value_bool(0);
    const uint8_t *pub = NULL;
    const uint8_t *msg = NULL;
    const uint8_t *sig = NULL;
    size_t pub_len = 0;
    size_t msg_len = 0;
    size_t sig_len = 0;
    if (!get_bytes(argv[0], &pub, &pub_len) || pub_len != 32) return value_bool(0);
    if (!get_bytes(argv[1], &msg, &msg_len)) return value_bool(0);
    if (!get_bytes(argv[2], &sig, &sig_len) || sig_len != 64) return value_bool(0);
    int ok = crypto_eddsa_check(sig, pub, msg, msg_len) == 0;
    return value_bool(ok);
}

static void ed25519_module_init(Env *global){
    lx_register_function("ed25519_keypair", n_ed25519_keypair);
    lx_register_function("ed25519_seed_keypair", n_ed25519_seed_keypair);
    lx_register_function("ed25519_public_key", n_ed25519_public_key);
    lx_register_function("ed25519_sign", n_ed25519_sign);
    lx_register_function("ed25519_verify", n_ed25519_verify);
    (void)global;
}

void register_ed25519_module(void) {
    lx_register_extension("ed25519");
    lx_register_module(ed25519_module_init);
}
