struct nssync_crypto_keybundle;

enum nssync_error nssync_crypto_synckey_encode(const uint8_t *key, char **key_out);

enum nssync_error nssync_crypto_synckey_decode(const char *key, uint8_t **key_out);

enum nssync_error nssync_crypto_new_sync_keybundle(uint8_t *sync_key, const char *accountname, struct nssync_crypto_keybundle **keybundle_out);

/** decrypt sync record
 *
 * verify hmac and decrypt data in json sync record
 *
 * @param record null terminated json string
 */
enum nssync_error nssync_crypto_decrypt_record(const char *record, struct nssync_auth *auth, struct nssync_crypto_keybundle *keybundle_out, uint8_t **plaintext_out, size_t *plaintext_length_out);
