# ed25519_keypair

Generate an Ed25519 key pair

Domain: Extensions: ed25519

---

### Description

`ed25519_keypair() : array`

Generates a new key pair using the system random source (`/dev/urandom`).
Returns an array with `public` and `secret` keys as blobs.

### Return Values

Returns `{public, secret}`, or `undefined` if randomness is unavailable.

### Examples

```lx
$kp = ed25519_keypair();
print(type($kp["public"]) . "\n");
```
