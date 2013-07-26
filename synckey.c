#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "nssync.h"

#include "base32.h"
#include "base64.h"
#include "hex.h"

#define SYNCKEY_LENGTH 16
#define BASE32_SYNCKEY_LENGTH 26
#define ENCODED_SYNCKEY_LENGTH 31

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

int nssync_crypto_synckey_encode(const uint8_t *key, char **key_out)
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
		return -1;
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

	return 0;
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

int nssync_crypto_synckey_decode(const char *key, uint8_t **key_out)
{
	uint8_t *synckey;
	char key32[BASE32_SYNCKEY_LENGTH + 1];
	int key32idx = 0;
	int keyidx = 0;
	int ret;

	synckey = malloc(SYNCKEY_LENGTH);
	if (synckey == NULL) {
		return -1;
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

	return 0;
}

static void dskey(const uint8_t *s)
{
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       s[0], s[1], s[2], s[3],
	       s[4], s[5], s[6], s[7],
	       s[8], s[9], s[10], s[11],
	       s[12], s[13], s[14], s[15]);
}

static void dkey(const uint8_t *s)
{
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
	       s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
	       s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23],
	       s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31]);
}

/** key bundle */
struct nssync_crypto_keybundle {
	uint8_t encryption[SHA256_DIGEST_LENGTH]; /* encryption key */
	uint8_t hmac[SHA256_DIGEST_LENGTH]; /* HMAC verification key */
};

/* generate sync key bundle from the sync key
 *
 * @param sync_key The binary sync key
 * @param 
 */
enum nssync_error
nssync_crypto_new_sync_keybundle(uint8_t *sync_key,
				 const char *accountname,
				 struct nssync_crypto_keybundle **keybundle_out)
{
	struct nssync_crypto_keybundle *keybundle;
	uint8_t data[128];
	int data_len;
	unsigned int encryption_key_len = SHA256_DIGEST_LENGTH;
	unsigned int hmac_key_len = SHA256_DIGEST_LENGTH;
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

int main(int argc, char **argv)
{
	const uint8_t example_synckey[SYNCKEY_LENGTH]= {
		0xc7,0x1a,0xa7,0xcb,
		0xd8,0xb8,0x2a,0x8f,
		0xf6,0xed,0xa5,0x5c,
		0x39,0x47,0x9f,0xd2
	};
	char *synckey_enc;
	uint8_t *synckey;
	const char *example_username="johndoe@example.com";
	struct nssync_crypto_keybundle *sync_keybundle;
//	const char *synckey_accountname = "vince@kyllikki.org";
	const char *synckey_accountname = "pnaksjwjnjiepjumadlhvtn44jrs44uf";
	const char *synckey_encoded = "i-xsxyz-wd3yj-5ytjx-9i7mj-wiwyy";
	const char *ciphertext_b64 = "Hpdf65sSxNzB6sbQzeAcp6CKRhN/mMi2WdM9c39rS2bDStkutQvMoW4l/hHOxAoRVgNWYKPYeY0LeYJX231xXvUqgw6o8/loO8tHxEMC8VQGR5hRuf0ya2ZgCqzarUGaCJljCBy981o8vIAEi26l0SX1XnqV6OAVVu9lKx+1TP+tZzYs0sDDHoKfG3tM8Cho/WRKemQWoGvW/mYs10jiKw==";
	const char *iv_b64="VmXHMMKy8mqVPpEfAlQ4vg==";
	const char *hmac_hex = "10462b667bba107d1334424117c2a5ce4f465a01c0a91c4f3e5827fd3bfb87d4";
	uint8_t *iv;
	uint8_t *hmac;

	printf("%s\n", example_username);

	dskey(example_synckey);

	nssync_crypto_synckey_encode(example_synckey, &synckey_enc);

	printf("%s\n", synckey_enc);

	nssync_crypto_synckey_decode(synckey_enc, &synckey);

	dskey(synckey);

	nssync_crypto_new_sync_keybundle(synckey, 
					 example_username, 
					 &sync_keybundle);

	dkey(sync_keybundle->encryption);
	dkey(sync_keybundle->hmac);

	free(sync_keybundle);
	free(synckey_enc);
	free(synckey);


	printf("%s\n", synckey_accountname);

	nssync_crypto_synckey_decode(synckey_encoded, &synckey);
	dskey(synckey);

	nssync_crypto_new_sync_keybundle(synckey,
					 synckey_accountname, 
					 &sync_keybundle);

	dkey(sync_keybundle->encryption);
	dkey(sync_keybundle->hmac);

	size_t output_length;
	iv = base64_decode((uint8_t *)iv_b64, strlen(iv_b64), &output_length);
	printf("iv length %d\n", output_length);
	hmac = hex16_decode((uint8_t *)hmac_hex, strlen(hmac_hex), &output_length);
	printf("hmac length %d\n", output_length);

	dskey(iv);
	dkey(hmac);

	unsigned int local_hmac_key_len = SHA256_DIGEST_LENGTH;
	uint8_t local_hmac[SHA256_DIGEST_LENGTH]; /* HMAC verification key */

	HMAC(EVP_sha256(), 
	     sync_keybundle->hmac, SHA256_DIGEST_LENGTH, 
	     (uint8_t *)ciphertext_b64, strlen(ciphertext_b64),
	     local_hmac, &local_hmac_key_len);

	dkey(local_hmac);

	free(sync_keybundle);
	free(synckey);

	printf("test encipher\n");

	const char *ex_ciphertext_b64 = "wcgqzENt5iXt9/7KPJ3rTA==";
	uint8_t ex_hmac_key[32] = { 0x2c,0x5d,0x98,0x09,0x2d,0x50,0x0a,0x04,0x8d,0x09,0xfd,0x01,0x09,0x0b,0xd0,0xd3,0xa4,0x86,0x1f,0xc8,0xea,0x24,0x38,0xbd,0x74,0xa8,0xf4,0x3b,0xe6,0xf4,0x7f,0x02 };
	uint8_t *ex_ciphertext;
	ex_ciphertext = base64_decode((uint8_t *)ex_ciphertext_b64, strlen(ex_ciphertext_b64), &output_length);

	dskey(ex_ciphertext);
	dkey(ex_hmac_key);

	HMAC(EVP_sha256(), 
	     ex_hmac_key, SHA256_DIGEST_LENGTH, 
	     (uint8_t *)ex_ciphertext_b64, strlen(ex_ciphertext_b64),
	     local_hmac, &local_hmac_key_len);

	dkey(local_hmac);





	return 0;
}

