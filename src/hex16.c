#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "hex16.h"

uint8_t *hex16_encode(const unsigned char *data,
		    size_t input_length,
		       size_t *output_length)
{
	return NULL;
}

static const uint8_t hextable[256] = {
	['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
	['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
	['A'] = 10, ['B'] = 11,	['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15,
	['a'] = 10, ['b'] = 11,	['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
};

uint8_t *hex16_decode(const uint8_t *data,
			     size_t input_length,
		       size_t *output_length)
{
	uint8_t *decoded;
	size_t decoded_length;
	size_t dloop;

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
