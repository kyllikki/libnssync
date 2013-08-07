#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include <jansson.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>

#include "nssync.h"

#include "crypto.h"
#include "base32.h"
#include "base64.h"
#include "hex16.h"

#define SYNCKEY_LENGTH 16
#define BASE32_SYNCKEY_LENGTH 26
#define ENCODED_SYNCKEY_LENGTH 31

#define ENCRYPTION_KEY_LENGTH SHA256_DIGEST_LENGTH
#define HMAC_KEY_LENGTH SHA256_DIGEST_LENGTH
#define IV_LENGTH 16

/** key bundle */
struct nssync_crypto_keybundle {
	uint8_t encryption[ENCRYPTION_KEY_LENGTH]; /* encryption key */
	uint8_t hmac[HMAC_KEY_LENGTH]; /* HMAC verification key */
};


static char tofriendly(int val)
{
	char c = tolower(val);
	if (c == 'l')
		c = '8';
	if (c == 'o')
		c = '9';
	if (c == '=')
		c = 0;
	return c;
}

static char fromfriendly(int val)
{
	char c = toupper(val);
	if (c == '8')
		c = 'L';
	if (c == '9')
		c = 'O';
	if (c == '-')
		c = 0;
	return c;
}


enum nssync_error
nssync_crypto_synckey_encode(const uint8_t *key, char **key_out)
{
	char *genkey;
	char key32[BASE32_SYNCKEY_LENGTH + 1];
	int key32idx = 0;
	size_t buflen = sizeof(key32);
	int keyidx = 0;
	int idx;

	base32_encode(key32, &buflen, key, SYNCKEY_LENGTH);

	genkey = malloc(ENCODED_SYNCKEY_LENGTH + 1); /* allow for zero pad */
	if (genkey == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	genkey[keyidx++] = tofriendly(key32[key32idx++]);
	while (keyidx < ENCODED_SYNCKEY_LENGTH) {
		genkey[keyidx++] = '-';
		for (idx=0; idx < 5; idx++) {
			genkey[keyidx++] = tofriendly(key32[key32idx++]);
		}
	}
	genkey[keyidx++] = 0;

	*key_out = genkey;

	return NSSYNC_ERROR_OK;
}


enum nssync_error
nssync_crypto_synckey_decode(const char *key, uint8_t **key_out)
{
	uint8_t *synckey;
	char key32[BASE32_SYNCKEY_LENGTH + 1];
	int key32idx = 0;
	int keyidx = 0;
	int ret;

	synckey = malloc(SYNCKEY_LENGTH);
	if (synckey == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	while ((key32idx < BASE32_SYNCKEY_LENGTH) &&
	       (key[keyidx] != 0)) {
		key32[key32idx] = fromfriendly(key[keyidx++]);
		if (key32[key32idx] != 0) {
			key32idx++;
		}
	}
	key32[key32idx] = 0;

	ret = base32_decode(synckey, SYNCKEY_LENGTH, key32, BASE32_SYNCKEY_LENGTH);

	if (ret != SYNCKEY_LENGTH) {
		free(synckey);
		return -2;
	}

	*key_out = synckey;

	return NSSYNC_ERROR_OK;
}


enum nssync_error
nssync_crypto_keybundle_new_b64(const char *key_b64,
				const char *hmac_b64,
				struct nssync_crypto_keybundle **keybundle_out)
{
	struct nssync_crypto_keybundle *keybundle;
	size_t key_length;
	size_t hmac_length;
	uint8_t *key;
	uint8_t *hmac;

	key = base64_decode((uint8_t *)key_b64,
			   strlen(key_b64),
			   &key_length);

	if ((key == NULL) || (key_length != ENCRYPTION_KEY_LENGTH)) {
		free(key);
		return NSSYNC_ERROR_PROTOCOL;
	}

	hmac = base64_decode((uint8_t *)hmac_b64,
			   strlen(hmac_b64),
			   &hmac_length);


	if ((hmac == NULL) || (hmac_length != HMAC_KEY_LENGTH)) {
		free(key);
		free(hmac);
		return NSSYNC_ERROR_PROTOCOL;
	}

	keybundle = calloc(1, sizeof(*keybundle));
	if (keybundle == NULL) {
		free(key);
		free(hmac);
		return NSSYNC_ERROR_NOMEM;
	}

	memcpy(keybundle->encryption, key, ENCRYPTION_KEY_LENGTH);
	free(key);

	memcpy(keybundle->hmac, hmac, HMAC_KEY_LENGTH);
	free(hmac);

	*keybundle_out = keybundle;

	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_crypto_keybundle_new_user_synckey(const char *user_synckey,
				 const char *accountname,
				 struct nssync_crypto_keybundle **keybundle_out)
{
	enum nssync_error ret;
	uint8_t *synckey;

	ret = nssync_crypto_synckey_decode(user_synckey, &synckey);
	if (ret != NSSYNC_ERROR_OK) {
		return ret;
	}

	ret = nssync_crypto_keybundle_new_synckey(synckey, accountname, keybundle_out);
	free(synckey);
	return ret;
}

enum nssync_error
nssync_crypto_keybundle_new_synckey(const uint8_t *sync_key,
			const char *accountname,
			struct nssync_crypto_keybundle **keybundle_out)
{
	struct nssync_crypto_keybundle *keybundle;
	uint8_t data[128];
	int data_len;
	unsigned int encryption_key_len = ENCRYPTION_KEY_LENGTH;
	unsigned int hmac_key_len = HMAC_KEY_LENGTH;
	const char *hmac_input = "Sync-AES_256_CBC-HMAC256";

	keybundle = calloc(1, sizeof(*keybundle));
	if (keybundle == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	/* build the data to be hashed with the HMAC process to
	 * generate encryption key part of bundle
	 */
	data_len = snprintf((char*)data, 128, "%s%s%c", hmac_input, accountname, 1);

	HMAC(EVP_sha256(),
	     sync_key, SYNCKEY_LENGTH,
	     data, data_len,
	     keybundle->encryption, &encryption_key_len);

	/* build the data to be hashed with the HMAC process to
	 * generate hmac key part of bundle
	 */
	memcpy(data, keybundle->encryption, encryption_key_len);
	data_len = encryption_key_len +
		snprintf((char*)data + encryption_key_len,
			 128 - encryption_key_len,
			 "%s%s%c",
			 hmac_input, accountname, 2);

	HMAC(EVP_sha256(),
	     sync_key, SYNCKEY_LENGTH,
	     data, data_len,
	     keybundle->hmac, &hmac_key_len);

	*keybundle_out = keybundle;
	return NSSYNC_ERROR_OK;
}


enum nssync_error
nssync_crypto_decrypt_record(const char *record,
			     struct nssync_crypto_keybundle *keybundle,
			     uint8_t **plaintext_out,
			     size_t *plaintext_length_out)
{
	/* json objects */
	json_t *root;
	json_error_t error;
	json_t *hmac_hex16_json;
	json_t *ciphertext_b64_json;
	json_t *iv_b64_json;

	/* text from json */
	const char *hmac_hex16;
	const char *ciphertext_b64;
	const char *iv_b64;

	/* HMAC from record */
	uint8_t *record_hmac;
	size_t record_hmac_length = HMAC_KEY_LENGTH;

	/* HMAC computed from key */
	unsigned int local_hmac_length = HMAC_KEY_LENGTH;
	uint8_t local_hmac[HMAC_KEY_LENGTH];

	/* decoded ciphertext */
	uint8_t *ciphertext;
	size_t ciphertext_length;
	uint8_t *iv;
	size_t iv_length;

	/* AES state */
	AES_KEY aeskey;

	/* decypted data */
	uint8_t *plaintext;

	/* json load */
	root = json_loads(record, 0, &error);
	if (!root) {
		debugf("error: on line %d of reply: %s\n",
			error.line, error.text);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if(!json_is_object(root)) {
		debugf("error: root is not an object\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* extract ciphertext from record (undecoded) */

	hmac_hex16_json = json_object_get(root, "hmac");
	ciphertext_b64_json = json_object_get(root, "ciphertext");
	iv_b64_json = json_object_get(root, "IV");
	if ((!json_is_string(hmac_hex16_json)) ||
	    (!json_is_string(ciphertext_b64_json)) ||
	    (!json_is_string(iv_b64_json))) {
		debugf("missing or incorrectly formatted fields in record\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}
	hmac_hex16 = json_string_value(hmac_hex16_json);
	ciphertext_b64 = json_string_value(ciphertext_b64_json);
	iv_b64 = json_string_value(iv_b64_json);

	/* hex16 decode hmac from record */
	record_hmac = hex16_decode((uint8_t *)hmac_hex16,
				   strlen(hmac_hex16),
				   &record_hmac_length);
	if (record_hmac_length != HMAC_KEY_LENGTH) {
		debugf("record hmac length %zu incorrect (should be %d)\n",
			record_hmac_length, HMAC_KEY_LENGTH);
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* calculate local hmac value */
	HMAC(EVP_sha256(),
	     keybundle->hmac, HMAC_KEY_LENGTH,
	     (uint8_t *)ciphertext_b64, strlen(ciphertext_b64),
	     local_hmac, &local_hmac_length);

	/* verify hmac */
	if (memcmp(record_hmac, local_hmac, SHA256_DIGEST_LENGTH) != 0) {
		debugf("record hmac does not match computed. bad key?\n");
		free(record_hmac);
		json_decref(root);
		return NSSYNC_ERROR_HMAC;
	}
	free(record_hmac);

	/* base64 decode iv from record */
	iv = base64_decode((uint8_t *)iv_b64,
			   strlen(iv_b64),
			   &iv_length);
	if ((iv == NULL) || (iv_length != IV_LENGTH)) {
		debugf("IV data was size %zu (expected %d)\n",
			iv_length, IV_LENGTH);
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* base64 decode ciphertext */
	ciphertext = base64_decode((uint8_t *)ciphertext_b64,
				   strlen(ciphertext_b64),
				   &ciphertext_length);
	if (ciphertext == NULL) {
		json_decref(root);
		free(iv);
		return NSSYNC_ERROR_NOMEM;
	}

	/* json unref */
	json_decref(root);

	/* decrypt data */
	plaintext = malloc(ciphertext_length + 1);
	if (plaintext == NULL) {
		free(ciphertext);
		free(iv);
		return NSSYNC_ERROR_NOMEM;
	}
	plaintext[ciphertext_length] = 0;

	AES_set_decrypt_key(keybundle->encryption, 256, &aeskey);
	AES_cbc_encrypt(ciphertext, plaintext, ciphertext_length, &aeskey, iv, AES_DECRYPT);

	free(ciphertext);
	free(iv);

	*plaintext_out = plaintext;
	if (plaintext_length_out != NULL) {
		*plaintext_length_out = ciphertext_length;
	}

	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_crypto_keybundle_get_encryption(struct nssync_crypto_keybundle *keybundle, uint8_t **encryption_out, size_t *encryption_length_out)
{
	*encryption_out = keybundle->encryption;
	if (encryption_length_out != NULL) {
		*encryption_length_out = ENCRYPTION_KEY_LENGTH;
	}

	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_crypto_keybundle_get_hmac(struct nssync_crypto_keybundle *keybundle, uint8_t **hmac_out, size_t *hmac_length_out)
{
	*hmac_out = keybundle->hmac;
	if (hmac_length_out != NULL) {
		*hmac_length_out = HMAC_KEY_LENGTH;
	}

	return NSSYNC_ERROR_OK;
}
