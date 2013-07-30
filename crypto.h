
struct nssync_crypto_keybundle;

/* encode binary sync key to user format */
enum nssync_error nssync_crypto_synckey_encode(const uint8_t *key, char **key_out);

/* decode sync key from user format to binary */
enum nssync_error nssync_crypto_synckey_decode(const char *key, uint8_t **key_out);

/** create a new sync key bundle from a binary sync key
 *
 * @param sync_key The binary sync key
 * @param
 */
enum nssync_error nssync_crypto_keybundle_new_synckey(const uint8_t *synckey, const char *accountname, struct nssync_crypto_keybundle **keybundle_out);

/** generate a sync key bundle from a user encoded sync key
 *
 * @param
 * @param
 */
enum nssync_error nssync_crypto_keybundle_new_user_synckey(const char *user_synckey, const char *accountname, struct nssync_crypto_keybundle **keybundle_out);

enum nssync_error nssync_crypto_keybundle_get_encryption(struct nssync_crypto_keybundle *keybundle, uint8_t **encryption_out, size_t *encryption_length_out);

enum nssync_error nssync_crypto_keybundle_get_hmac(struct nssync_crypto_keybundle *keybundle, uint8_t **hmac_out, size_t *hmac_length_out);

/** decrypt sync record
 *
 * verify hmac and decrypt data in json sync record
 *
 * @param record null terminated json string
 */
enum nssync_error nssync_crypto_decrypt_record(const char *record, struct nssync_crypto_keybundle *keybundle, uint8_t **plaintext_out, size_t *plaintext_length_out);
