/*
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnssync, http://www.netsurf-browser.org/
 *
 * Released under the Expat MIT License (see COPYING),
 *
 * This implements access to the mozilla registration service
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>

#include <openssl/sha.h>

#include <nssync/error.h>
#include <nssync/fetcher.h>

#include "util.h"
#include "base32.h"
#include "registration.h"

#define WEAVE_PATH "%suser/1.0/%s/node/weave"

struct nssync_registration {
	nssync_fetcher *fetcher;

	char *server; /* registration server */
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
static char *moz_sync_username_from_accountname(const char *accountname)
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
		base32_encode((uint8_t*)username, &bufflen, digest, sizeof(digest));
	}

	return username;
}

/* exported interface documented in registration.h */
enum nssync_error
nssync_registration_new(const char *server,
			const char *account,
			const char *password,
			nssync_fetcher *fetcher,
			struct nssync_registration **reg_out)
{
	struct nssync_registration *newreg;

	newreg = calloc(1, sizeof(*newreg));
	if (newreg == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	newreg->fetcher = fetcher;
	newreg->server = strdup(server);
	newreg->account = strdup(account);
	newreg->password = strdup(password);

	newreg->username = moz_sync_username_from_accountname(account);
	if (newreg->username == NULL) {
		nssync_registration_free(newreg);
		return NSSYNC_ERROR_NOMEM;
	}

	*reg_out = newreg;
	return NSSYNC_ERROR_OK;
}

/* exported interface documented in registration.h */
enum nssync_error
nssync_registration_free(struct nssync_registration *reg)
{
	free(reg->username);
	free(reg->server);
	free(reg->account);
	free(reg->password);

	free(reg);

	return NSSYNC_ERROR_OK;
}


/* exported interface documented in registration.h */
char *
nssync_registration_get_storage_server(struct nssync_registration *reg)
{
	struct nssync_fetcher_fetch fetch = { };

	if (reg->storage_server == NULL) {
		if (nssync__saprintf(&fetch.url, WEAVE_PATH, reg->server, reg->username) >= 0) {
			fetch.username = reg->username;
			fetch.password = reg->password;
			if (reg->fetcher(&fetch) == NSSYNC_ERROR_OK) {
				reg->storage_server = fetch.data;
			}
			free(fetch.url);
		}
	}

	return reg->storage_server;
}

char *
nssync_registration_get_username(struct nssync_registration *reg)
{
	return reg->username;
}

char *
nssync_registration_get_password(struct nssync_registration *reg)
{
	return reg->password;
}
