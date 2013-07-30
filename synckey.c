#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>

#include "nssync.h"
#include "crypto.h"

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

void spec_synckey_coding_test(void)
{
	const uint8_t orig_synckey[] = {
		0xc7,0x1a,0xa7,0xcb,0xd8,0xb8,0x2a,0x8f,
		0xf6,0xed,0xa5,0x5c,0x39,0x47,0x9f,0xd2
	};
	const char *orig_synckey_user = "y-4nkps-6yxav-i75xn-uv9ds-r472i";

	char *synckey_user;
	uint8_t *synckey;

	printf("Sync key encoding:");
	if (nssync_crypto_synckey_encode(orig_synckey, &synckey_user) == NSSYNC_ERROR_OK) {

		if (strcmp(synckey_user, orig_synckey_user) != 0) {
			printf("incorrect value\ngenerated %s (should be %s)\n",
			       synckey_user, orig_synckey_user);
		} else {
			printf("passed\n");
		}
		free(synckey_user);
	} else {
		printf("failed\n");
	}


	printf("Sync key decoding:");
	if (nssync_crypto_synckey_decode(orig_synckey_user, &synckey) == NSSYNC_ERROR_OK) {
		if (memcmp(orig_synckey, synckey, 16) != 0) {
			printf("incorrect value\n");
			dskey(synckey);
			dskey(orig_synckey);
		} else {
			printf("passed\n");
		}
		free(synckey);
	} else {
		printf("failed\n");
	}
}

void spec_sync_keybundle_test(void)
{
	const char *orig_username = "johndoe@example.com"; /* this example is badly misleading as the shar256 base32 username must be used in reality */
	const uint8_t orig_synckey[] = {
		0xc7,0x1a,0xa7,0xcb,0xd8,0xb8,0x2a,0x8f,
		0xf6,0xed,0xa5,0x5c,0x39,0x47,0x9f,0xd2
	};
	const uint8_t orig_hmac[] = {
		0xbf,0x9e,0x48,0xac,0x50,0xa2,0xfc,0xc4,0x00,0xae,0x4d,0x30,0xa5,0x8d,0xc6,0xa8,0x3a,0x77,0x20,0xc3,0x2f,0x58,0xc6,0x0f,0xd9,0xd0,0x2d,0xb1,0x6e,0x40,0x62,0x16
	};
	const uint8_t orig_encryption[] = {
		0x8d,0x07,0x65,0x43,0x0e,0xa0,0xd9,0xdb,0xd5,0x3c,0x53,0x6c,0x6c,0x5c,0x4c,0xb6,0x39,0xc0,0x93,0x07,0x5e,0xf2,0xbd,0x77,0xcd,0x30,0xcf,0x48,0x51,0x38,0xb9,0x05
	};

	struct nssync_crypto_keybundle *sync_keybundle;
	uint8_t *encryption;
	uint8_t *hmac;
	size_t length;

	printf("Sync keybundle creation:");

	if (nssync_crypto_keybundle_new_synckey(orig_synckey,
						orig_username,
						&sync_keybundle) == NSSYNC_ERROR_OK) {
		printf("passed\nEncryption value:");

		nssync_crypto_keybundle_get_encryption(sync_keybundle,
						       &encryption,
						       &length);
		if (memcmp(orig_encryption, encryption, length) != 0) {
			printf("incorrect\n");
			dkey(encryption);
			dkey(orig_encryption);
		} else {
			printf("correct\n");
		}

		printf("hmac value:");

		nssync_crypto_keybundle_get_hmac(sync_keybundle,
						 &hmac,
						 &length);
		if (memcmp(orig_hmac, hmac, length) != 0) {
			printf("incorrect\n");
			dkey(hmac);
			dkey(orig_hmac);
		} else {
			printf("correct\n");
		}

		free(sync_keybundle);
	} else {
		printf("failed\n");
	}
}

enum nssync_error local_sync_key_record_decode(void)
{
	const char *username = "pnaksjwjnjiepjumadlhvtn44jrs44uf";
	const char *synckey_user = "i-xsxyz-wd3yj-5ytjx-9i7mj-wiwyy";
	const char *record = "{\"ciphertext\":\"Hpdf65sSxNzB6sbQzeAcp6CKRhN/mMi2WdM9c39rS2bDStkutQvMoW4l/hHOxAoRVgNWYKPYeY0LeYJX231xXvUqgw6o8/loO8tHxEMC8VQGR5hRuf0ya2ZgCqzarUGaCJljCBy981o8vIAEi26l0SX1XnqV6OAVVu9lKx+1TP+tZzYs0sDDHoKfG3tM8Cho/WRKemQWoGvW/mYs10jiKw==\",\"IV\":\"VmXHMMKy8mqVPpEfAlQ4vg==\",\"hmac\":\"10462b667bba107d1334424117c2a5ce4f465a01c0a91c4f3e5827fd3bfb87d4\"}";
	uint8_t *synckey;
	struct nssync_crypto_keybundle *sync_keybundle;
	enum nssync_error ret;
	uint8_t *plaintext;
	size_t plaintext_length;

	printf("Sync keybundle create:");

	ret = nssync_crypto_keybundle_new_user_synckey(synckey_user, username, &sync_keybundle);
	if (ret != NSSYNC_ERROR_OK) {
		printf("failed\n");
		return ret;
	}
	printf("ok\n");

	printf("Record decryption:");
	ret = nssync_crypto_decrypt_record(record, sync_keybundle, &plaintext, &plaintext_length);
	free(sync_keybundle);
	if (ret != NSSYNC_ERROR_OK) {
		printf("failed\n");
		return ret;
	}
	printf("ok\n");

	printf("%d:%s\n", plaintext_length, plaintext);
	
	return ret;
}

int main(int argc, char **argv)
{
	spec_synckey_coding_test();
	spec_sync_keybundle_test();
	local_sync_key_record_decode();

#if 0
	char *synckey_enc;
	uint8_t *synckey;
	struct nssync_crypto_keybundle *sync_keybundle;
//	const char *synckey_accountname = "vince@kyllikki.org";
	const char *synckey_username = "pnaksjwjnjiepjumadlhvtn44jrs44uf";
	const char *synckey_encoded = "i-xsxyz-wd3yj-5ytjx-9i7mj-wiwyy";
	const char *ciphertext_b64 = "Hpdf65sSxNzB6sbQzeAcp6CKRhN/mMi2WdM9c39rS2bDStkutQvMoW4l/hHOxAoRVgNWYKPYeY0LeYJX231xXvUqgw6o8/loO8tHxEMC8VQGR5hRuf0ya2ZgCqzarUGaCJljCBy981o8vIAEi26l0SX1XnqV6OAVVu9lKx+1TP+tZzYs0sDDHoKfG3tM8Cho/WRKemQWoGvW/mYs10jiKw==";
	const char *iv_b64="VmXHMMKy8mqVPpEfAlQ4vg==";
	const char *hmac_hex = "10462b667bba107d1334424117c2a5ce4f465a01c0a91c4f3e5827fd3bfb87d4";
	uint8_t *iv;
	uint8_t *record_hmac;
	const char *ex_ciphertext_b64 = "wcgqzENt5iXt9/7KPJ3rTA==";
	uint8_t ex_hmac_key[32] = { 0x2c,0x5d,0x98,0x09,0x2d,0x50,0x0a,0x04,0x8d,0x09,0xfd,0x01,0x09,0x0b,0xd0,0xd3,0xa4,0x86,0x1f,0xc8,0xea,0x24,0x38,0xbd,0x74,0xa8,0xf4,0x3b,0xe6,0xf4,0x7f,0x02 };
	uint8_t *ciphertext;
	size_t output_length;
	unsigned int local_hmac_key_len = SHA256_DIGEST_LENGTH;
	uint8_t local_hmac[SHA256_DIGEST_LENGTH]; /* HMAC verification key */




	printf("\ntest encipher\n");


	ciphertext = base64_decode((uint8_t *)ex_ciphertext_b64,
				   strlen(ex_ciphertext_b64),
				   &output_length);

	dskey(ciphertext);
	dkey(ex_hmac_key);

	HMAC(EVP_sha256(),
	     ex_hmac_key, SHA256_DIGEST_LENGTH,
	     (uint8_t *)ex_ciphertext_b64, strlen(ex_ciphertext_b64),
	     local_hmac, &local_hmac_key_len);

	dkey(local_hmac);

	free(ciphertext);

	printf("\nusername: %s\n", synckey_username);

	nssync_crypto_synckey_decode(synckey_encoded, &synckey);
	dskey(synckey);

	nssync_crypto_new_sync_keybundle(synckey,
					 synckey_username,
					 &sync_keybundle);

	dkey(sync_keybundle->encryption);
	dkey(sync_keybundle->hmac);

	record_hmac = hex16_decode((uint8_t *)hmac_hex,
				   strlen(hmac_hex),
				   &output_length);
	printf("hmac length %d\n", output_length);


	HMAC(EVP_sha256(),
	     sync_keybundle->hmac, SHA256_DIGEST_LENGTH,
	     (uint8_t *)ciphertext_b64, strlen(ciphertext_b64),
	     local_hmac, &local_hmac_key_len);


	if (memcmp(record_hmac, local_hmac, SHA256_DIGEST_LENGTH) != 0) {
		printf("record hmac does not match computed.sync key bad? %d\n",
		       local_hmac_key_len);
		dkey(record_hmac);
		dkey(local_hmac);
	} else {
		int ciphertext_length;
		char *outbuf;
		AES_KEY aeskey;

		iv = base64_decode((uint8_t *)iv_b64,
				   strlen(iv_b64),
				   &output_length);
		printf("iv length %d\n", output_length);
		dskey(iv);

		ciphertext = base64_decode((uint8_t *)ciphertext_b64,
					   strlen(ciphertext_b64),
					   &ciphertext_length);

		outbuf = calloc(1, ciphertext_length);

		AES_set_decrypt_key(sync_keybundle->encryption, 256, &aeskey);
		AES_cbc_encrypt(ciphertext, outbuf, ciphertext_length, &aeskey, iv, AES_DECRYPT);
		int plaintext_len;
		plaintext_len = strlen(outbuf);

		printf("%s\n%02x %02x\n", outbuf, outbuf[plaintext_len-2], outbuf[plaintext_len-1]);

		free(ciphertext);
	}
	free(sync_keybundle);
	free(synckey);




#endif

	return 0;
}
