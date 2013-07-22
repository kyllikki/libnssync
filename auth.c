#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include <openssl/sha.h>

#include "base32.h"
#include "request.h"

#include "nssync_error.h"
#include "auth.h"

struct nssync_auth {
	char *server; /* auth server */
	char *account; /* users account name */
	char *password; /* users account password */

	char *username; /* username used with services (not account!) */
	char *storage_server; /* storage server */
};


static bool isvalidusername(const char *s)
{
	const char *match = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";

	while (*s != 0) {
		if (strchr(match, *s) == NULL) {
			return false;
		}
		s++;
	}
	return true;
}

static char *strduptolower(const char *s) {
	char *ret;
	int sloop = 0;

	ret = malloc(strlen(s) + 1);
	if (ret != NULL) {
		while (s[sloop] != 0) {
			ret[sloop] = tolower(s[sloop]);
			sloop++;
		}
		ret[sloop] = 0;
	}
	return ret;
}

/** create a username from an account name
 *
 * The requirements for username are not explained anywhere except
 * except in the source code at
 * https://hg.mozilla.org/mozilla-central/file/2cfff9240e9a/services/sync/modules/identity.js#l422
 *
 */
char *moz_sync_username_from_accountname(const char *accountname)
{
	char *username;
	SHA_CTX context;
	uint8_t digest[20];
	size_t bufflen = 80;

	if (isvalidusername(accountname)) {
		return strduptolower(accountname);
	}

	SHA1_Init(&context);
	SHA1_Update(&context, accountname, strlen(accountname));
	SHA1_Final(digest, &context);

	username = calloc(1, bufflen);
	if (username != NULL) {
		base32_encode(username, &bufflen, digest, sizeof(digest));
	}

	return username;
}

/* exported interface documented in auth.h */
enum nnsync_error
nssync_auth_new(const char *server,
		const char *account,
		const char *password,
		struct nssync_auth **auth_out)
{
	struct nssync_auth *newauth;

	newauth = calloc(1, sizeof(*newauth));
	if (newauth == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	newauth->server = strdup(server);
	newauth->account = strdup(account);
	newauth->password = strdup(password);

	newauth->username = moz_sync_username_from_accountname(account);
	if (newauth->username == NULL) {
		nssync_auth_free(newauth);
		return NSSYNC_ERROR_NOMEM;
	}

	*auth_out = newauth;
	return NSSYNC_ERROR_OK;
}

/* exported interface documented in auth.h */
enum nnsync_error
nssync_auth_free(struct nssync_auth *auth)
{
	free(auth->username);
	free(auth->server);
	free(auth->account);
	free(auth->password);

	free(auth);

	return NSSYNC_ERROR_OK;
}

#define URL_SIZE 256
#define AUTH_PATH "%suser/1.0/%s/node/weave"

/* exported interface documented in auth.h */
char *
nssync_auth_get_storage_server(struct nssync_auth *auth)
{
	char url[URL_SIZE];

	if (auth->storage_server == NULL) {
		snprintf(url, URL_SIZE, AUTH_PATH, auth->server, auth->username);
		auth->storage_server = nssync__request(url, NULL, NULL);
	}

	return auth->storage_server;
}

char *
nssync_auth_get_username(struct nssync_auth *auth)
{
	return auth->username;
}

char *
nssync_auth_get_password(struct nssync_auth *auth)
{
	return auth->password;
}
