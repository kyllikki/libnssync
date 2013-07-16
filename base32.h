int 
base32_encode(char *buf, size_t *buflen, const void *data, size_t size);

int
base32_decode(void *buf, size_t *buflen, const char *str, size_t slen);
int b32_5to8(int);
int b32_8to5(int);
