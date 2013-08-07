/*
 * Fills *buf with max. *buflen characters, encoding size bytes of *data.
 *
 * NOTE: *buf space should be at least 1 byte _more_ than *buflen
 * to hold the trailing '\0'.
 *
 * return value    : #bytes filled in buf   (excluding \0)
 * sets *buflen to : #bytes encoded from data
 */
int base32_encode(char *buf, size_t *buflen, const void *data, size_t size);

/*
 * Fills *buf with max. buflen bytes, decoded from slen chars in *str.
 * Decoding stops early when *str contains \0.
 * Illegal encoded chars are assumed to decode to zero.
 *
 * return value    : #bytes filled in buf   (excluding \0)
 */
int base32_decode(void *buf, size_t buflen, const char *str, size_t slen);

int b32_5to8(int);
int b32_8to5(int);
