# ed25519_public_key

Derive the Ed25519 public key from a secret key

Domain: Extensions: ed25519

---

### Description

`ed25519_public_key(secret) : blob|undefined`

Returns the public key from a 64-byte Ed25519 secret key.

### Parameters

- **`secret`**: 64-byte secret key (string or blob).

### Return Values

Returns the 32-byte public key as a blob, or `undefined` on invalid input.

### Examples

```lx
$kp = ed25519_seed_keypair("0123456789abcdef0123456789abcdef");
$pub = ed25519_public_key($kp["secret"]);
```
