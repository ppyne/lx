# ed25519_seed_keypair

Generate an Ed25519 key pair from a seed

Domain: Extensions: ed25519

---

### Description

`ed25519_seed_keypair(seed) : array`

Generates a deterministic key pair from a 32-byte seed.
Returns an array with `public` and `secret` keys as blobs.

### Parameters

- **`seed`**: 32-byte seed (string or blob).

### Return Values

Returns `{public, secret}`, or `undefined` on invalid input.

### Examples

```lx
$seed = "0123456789abcdef0123456789abcdef";
$kp = ed25519_seed_keypair($seed);
```
