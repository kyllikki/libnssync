#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sha1.h"
#include "base32.h"

/* not explained anywhere ecept in teh sourec code at https://hg.mozilla.org/mozilla-central/file/2cfff9240e9a/services/sync/modules/identity.js#l422 */
int moz_sync_username_from_accountname(accountname)
{
// If we encounter characters not allowed by the API (as found for instance in an email address), hash the value.
//if (value && value.match(/[^A-Z0-9._-]/i)) { 
//return Utils.sha1Base32(value.toLowerCase()).toLowerCase();
//}
//return value ? value.toLowerCase() : value;
}

int main(int argc, char **argv)
{
    SHA1_CTX context;
    uint8_t digest[20];
    char output[80];
    size_t bufflen = sizeof(output);

    SHA1_Init(&context);
    SHA1_Update(&context, argv[1], strlen(argv[1]));
    SHA1_Final(&context, digest);

    base32_encode(output, &bufflen, digest, sizeof(digest));

    printf("%s\n", output);
//    digest_to_hex(digest, output);


}
