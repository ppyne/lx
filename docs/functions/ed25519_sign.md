# ed25519_sign

Sign a message with Ed25519

Domain: Extensions: ed25519

---

### Description

`ed25519_sign(secret, message) : blob|undefined`

Signs a message with a 64-byte Ed25519 secret key and returns a 64-byte
signature as a blob.

### Parameters

- **`secret`**: 64-byte secret key (string or blob).
- **`message`**: Message data (string or blob).

### Return Values

Returns the signature as a blob, or `undefined` on invalid input.

### Examples

```lx
$kp = ed25519_seed_keypair("0123456789abcdef0123456789abcdef");
$sig = ed25519_sign($kp["secret"], "hello");
```
