# ed25519_verify

Verify an Ed25519 signature

Domain: Extensions: ed25519

---

### Description

`ed25519_verify(public, message, signature) : bool`

Verifies a 64-byte signature against a message and 32-byte public key.

### Parameters

- **`public`**: 32-byte public key (string or blob).
- **`message`**: Message data (string or blob).
- **`signature`**: 64-byte signature (string or blob).

### Return Values

Returns `true` when valid, `false` otherwise.

### Examples

```lx
$kp = ed25519_seed_keypair("0123456789abcdef0123456789abcdef");
$sig = ed25519_sign($kp["secret"], "hello");
print(ed25519_verify($kp["public"], "hello", $sig) . "\n");
```
