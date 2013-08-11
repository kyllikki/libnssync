#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <openssl/sha.h>

/* #include "sha1.h"*/
#include "base32.h"


int main(int argc, char **argv)
{
	SHA_CTX context;
	uint8_t digest[20];
	char output[80];
	size_t bufflen = sizeof(output);

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s <value>\n", argv[0]);
		return 1;
	}

	SHA1_Init(&context);
	SHA1_Update(&context, argv[1], strlen(argv[1]));
	SHA1_Final(digest, &context);

	base32_encode((uint8_t *)output, &bufflen, digest, sizeof(digest));

	printf("%s\n", output);

	return 0;

}
