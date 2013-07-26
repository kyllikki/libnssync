#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "hex.h"

uint8_t *hex16_encode(const unsigned char *data,
                    size_t input_length,
		       size_t *output_length)
{
	return NULL;
}

static const uint8_t hextable[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0, /* 0-9*/
	0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, /* A-F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0, /* a-f */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

uint8_t *hex16_decode(const uint8_t *data,
                             size_t input_length,
		       size_t *output_length)
{
	uint8_t *decoded;
	size_t decoded_length;
	int dloop;

	decoded_length = input_length / 2; /* 4 bits per input byte */
	decoded = malloc(decoded_length);

	if (decoded == NULL) {
		return NULL;
	}

	for (dloop = 0; dloop < decoded_length; dloop++) {
		decoded[dloop] = (hextable[*(data)] << 4) | hextable[*(data + 1)];
		data+=2;
	}

	*output_length = decoded_length;
	return decoded;
}
