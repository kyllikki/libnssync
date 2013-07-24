#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "base32.h"

#define SYNC_KEY_LEN 16
#define BASE32_SYNC_KEY_LEN 26
#define ENCODED_SYNC_KEY_LEN 31

char tofriendly(int val)
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

int encode_sync_key(uint8_t *key, char **key_out)
{
	char *genkey;
	char key32[BASE32_SYNC_KEY_LEN + 1];
	int key32idx = 0;
	size_t buflen = sizeof(key32);
	int keyidx = 0;
	int idx;

	base32_encode(key32, &buflen, key, SYNC_KEY_LEN);

	genkey = malloc(ENCODED_SYNC_KEY_LEN + 1); /* allow for zero pad */
	if (genkey == NULL) {
		return -1;
	}

	genkey[keyidx++] = tofriendly(key32[key32idx++]);
	while (keyidx < ENCODED_SYNC_KEY_LEN) {
		genkey[keyidx++] = '-';
		for (idx=0; idx < 5; idx++) {
			genkey[keyidx++] = tofriendly(key32[key32idx++]);
		}
	}
	genkey[keyidx++] = 0;
	*key_out = genkey;

	return 0;
}

char fromfriendly(int val)
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

int decode_sync_key(char *key, uint8_t **key_out)
{
	uint8_t *synckey;
	char key32[BASE32_SYNC_KEY_LEN + 1];
	int key32idx = 0;
	int keyidx = 0;
	int ret;

	synckey = malloc(SYNC_KEY_LEN);
	if (synckey == NULL) {
		return -1;
	}

	while ((key32idx < BASE32_SYNC_KEY_LEN) &&
	       (key[keyidx] != 0)) {
		key32[key32idx] = fromfriendly(key[keyidx++]);
		if (key32[key32idx] != 0) {
			key32idx++;
		}
	}
	key32[key32idx] = 0;

	ret = base32_decode(synckey, SYNC_KEY_LEN, key32, BASE32_SYNC_KEY_LEN);

	if (ret != SYNC_KEY_LEN) {
		free(synckey);
		return -2;
	}
	*key_out = synckey;

	return 0;
}

static void dskey(uint8_t *s)
{
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       s[0], s[1], s[2], s[3],
	       s[4], s[5], s[6], s[7],
	       s[8], s[9], s[10], s[11],
	       s[12], s[13], s[14], s[15]);
}

static void dkey(uint8_t *s)
{
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
	       s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
	       s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23],
	       s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31]);
}

int main(int argc, char **argv)
{
	uint8_t synckey[SYNC_KEY_LEN]= {
		0xc7,0x1a,0xa7,0xcb,
		0xd8,0xb8,0x2a,0x8f,
		0xf6,0xed,0xa5,0x5c,
		0x39,0x47,0x9f,0xd2
	};
	char *synckey_enc;
	uint8_t *synckey_d;
	const char *username="johndoe@example.com";
	const char *hmac_input = "Sync-AES_256_CBC-HMAC256";
//	const uint8_t *info="Sync-AES_256_CBC-HMAC256johndoe@example.com\x01";
	char data[128];
	int data_len;
	uint8_t encryption_key[SHA256_DIGEST_LENGTH];
	unsigned int encryption_key_len = SHA256_DIGEST_LENGTH;

	uint8_t hmac_key[SHA256_DIGEST_LENGTH];
	unsigned int hmac_key_len = SHA256_DIGEST_LENGTH;

	dskey(synckey);

	encode_sync_key(synckey, &synckey_enc);

	printf("%s\n", synckey_enc);

	decode_sync_key(synckey_enc, &synckey_d);

	dskey(synckey_d);

	data_len = snprintf(data, 128, "%s%s%c", hmac_input, username, 1);

	HMAC(EVP_sha256(), synckey_d, SYNC_KEY_LEN, data, data_len, encryption_key, &encryption_key_len);

	dkey(encryption_key);

	memcpy(data, encryption_key, encryption_key_len);
	data_len = encryption_key_len + snprintf(data + encryption_key_len, 
			    128 - encryption_key_len, 
			    "%s%s%c", hmac_input, username, 2);
	HMAC(EVP_sha256(), synckey_d, SYNC_KEY_LEN, data, data_len, hmac_key, &hmac_key_len);


	dkey(hmac_key);


	free(synckey_enc);
	free(synckey_d);

	return 0;
}

