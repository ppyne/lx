# aead_encrypt

Encrypt data with ChaCha20-Poly1305 (AEAD)

Domain: Extensions: aead

---

### Description

`aead_encrypt(key, nonce, plaintext[, aad]) : blob`

Encrypts `plaintext` using ChaCha20-Poly1305 and returns a `blob` containing
`ciphertext || tag` (16-byte tag appended). The key must be 32 bytes and the
nonce must be 24 bytes (XChaCha20-Poly1305).

### Parameters

- **`key`**: 32-byte key (string or blob).
- **`nonce`**: 24-byte nonce (string or blob).
- **`plaintext`**: Data to encrypt (string or blob).
- **`aad`**: Additional authenticated data (optional, string or blob).

### Return Values

Returns a `blob` containing `ciphertext || tag`, or `undefined` on invalid input.

Use `blob` values when your key, nonce, or data contains `0x00` bytes.

### Examples

```lx
$key = "0123456789abcdef0123456789abcdef";
$nonce = "abcdefghijklmnopqrstuvwx";
$cipher = aead_encrypt($key, $nonce, "hello");
```
