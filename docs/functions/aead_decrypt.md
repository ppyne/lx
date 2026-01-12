# aead_decrypt

Decrypt data with ChaCha20-Poly1305 (AEAD)

Domain: Extensions: aead

---

### Description

`aead_decrypt(key, nonce, ciphertext[, aad]) : blob|undefined`

Decrypts a `ciphertext || tag` blob produced by `aead_encrypt`. The key must be
32 bytes and the nonce must be 24 bytes (XChaCha20-Poly1305).

### Parameters

- **`key`**: 32-byte key (string or blob).
- **`nonce`**: 24-byte nonce (string or blob).
- **`ciphertext`**: Ciphertext with 16-byte tag appended (string or blob).
- **`aad`**: Additional authenticated data (optional, string or blob).

### Return Values

Returns a `blob` on success, or `undefined` on authentication failure or invalid input.

Use `blob` values when your key, nonce, or data contains `0x00` bytes.

### Examples

```lx
$key = "0123456789abcdef0123456789abcdef";
$nonce = "abcdefghijklmnopqrstuvwx";
$cipher = aead_encrypt($key, $nonce, "hello");
$plain = aead_decrypt($key, $nonce, $cipher);
print(str($plain) . "\n");
```
